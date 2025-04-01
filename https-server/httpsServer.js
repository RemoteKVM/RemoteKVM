const express = require('express');
const https = require('https');
const fs = require('fs');
const path = require('path');
const mysql = require('mysql2/promise');
const bcrypt = require('bcryptjs');
const app = express();
const bodyParser = require('body-parser');
require('dotenv').config();
const jwt = require('jsonwebtoken');
const helmet = require('helmet');
const crypto = require('crypto');

// www to non-www redirect middleware
app.use((req, res, next) => {
  const host = req.headers.host;
  
  // Redirect www to non-www
  if (host && host.startsWith('www.')) {
    const newHost = host.substring(4); // Remove 'www.'
    return res.redirect(301, `https://${newHost}${req.url}`);
  }
  
  next();
});

// Serve static files from React build
app.use(express.static(path.join(__dirname, './remotekvm-frontend/build')));

// Database Configuration
const dbConfig = {
  host: process.env.DB_HOST,
  user: process.env.DB_USER,
  password: process.env.DB_PASSWORD,
  database: process.env.DB_NAME,
  port: process.env.DB_PORT || 3306,
  ssl: process.env.DB_SSL === 'true' ? {
    rejectUnauthorized: false // Required for Azure MySQL with SSL
  } : false,
};

// Database Connection Pool
const pool = mysql.createPool(dbConfig);

// Use middlewares
app.use(bodyParser.json());
app.use(helmet({
  contentSecurityPolicy: {
    directives: {
      defaultSrc: ["'self'"],
      connectSrc: ["'self'", "wss://remotekvm.online:2000"],
      scriptSrc: ["'self'", "'unsafe-inline'", "'unsafe-eval'"],
      styleSrc: ["'self'", "'unsafe-inline'", "https://fonts.googleapis.com"],
      fontSrc: ["'self'", "https://fonts.gstatic.com", "data:"],
      imgSrc: ["'self'", "data:"],
      baseUri: ["'self'"],
      formAction: ["'self'"]
    }
  }
}));

// Set caching headers for static assets
app.use(express.static(path.join(__dirname, './remotekvm-frontend/build'), {
  maxAge: '1y',  // Cache static assets for 1 year
  setHeaders: (res, path) => {
    // Different caching strategies based on file types
    if (path.endsWith('.html')) {
      // Don't cache HTML files
      res.setHeader('Cache-Control', 'no-cache');
    } else if (path.match(/\.(jpg|jpeg|png|gif|webp|svg|ico)$/i)) {
      // Aggressive caching for images including your logo
      res.setHeader('Cache-Control', 'public, max-age=31536000, immutable');
    } else if (path.match(/\.(js|css)$/i)) {
      // Good caching for JS and CSS (they have hashed filenames in production build)
      res.setHeader('Cache-Control', 'public, max-age=31536000, immutable');
    }
  }
}));

async function initializeDatabase() {
  try {
    const connection = await pool.getConnection();
    console.log('Connected to the MySQL database');
    
    // Create users table if not exists
    await connection.query(`
      CREATE TABLE IF NOT EXISTS users (
        id INT AUTO_INCREMENT PRIMARY KEY,
        username VARCHAR(50) NOT NULL UNIQUE,
        password VARCHAR(255) NOT NULL,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        last_login TIMESTAMP NULL
      )
    `);
    console.log('Users table initialized');
    
    // Create VMs table if not exists - note no RAM size as it's not modifiable
    await connection.query(`
      CREATE TABLE IF NOT EXISTS vms (
        id INT AUTO_INCREMENT PRIMARY KEY,
        user_id INT NOT NULL,
        vm_name VARCHAR(50) NOT NULL,
        disk_size INT NOT NULL DEFAULT 10,
        status VARCHAR(20) NOT NULL DEFAULT 'stopped',
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        last_active TIMESTAMP NULL,
        FOREIGN KEY (user_id) REFERENCES users(id)
      )
    `);
    console.log('VMs table initialized');
    
    // Create terminal tokens table
    await connection.query(`
      CREATE TABLE IF NOT EXISTS terminal_tokens (
        id INT AUTO_INCREMENT PRIMARY KEY,
        vm_id INT NOT NULL,
        token VARCHAR(64) NOT NULL UNIQUE,
        expires_at TIMESTAMP NOT NULL,
        FOREIGN KEY (vm_id) REFERENCES vms(id) ON DELETE CASCADE
      )
    `);
    console.log('Terminal tokens table initialized');

    connection.release();
  } catch (error) {
    console.error('Database initialization error:', error);
    process.exit(1);
  }
}

// Function to generate a random token
function generateToken(length = 32) {
  return crypto.randomBytes(length).toString('hex');
}

// Initialize the database on startup
initializeDatabase();

// Helper function to execute database queries
async function executeQuery(query, params = []) {
  try {
    const [results] = await pool.query(query, params);
    return results;
  } catch (error) {
    console.error('Database query error:', error);
    throw error;
  }
}

// Password hashing function
async function hashPassword(password) {
  const saltRounds = 10;
  return await bcrypt.hash(password, saltRounds);
}

// Password verification function
async function verifyPassword(password, hashedPassword) {
  return await bcrypt.compare(password, hashedPassword);
}


const JWT_SECRET = process.env.JWT_SECRET;
const JWT_EXPIRATION = process.env.JWT_EXPIRATION || '2h';

// Verify the secret is available
if (!JWT_SECRET) {
  console.error('JWT_SECRET is not defined in environment variables. This is a security risk.');
  process.exit(1);
}

const authenticateToken = (req, res, next) => {
  const authHeader = req.headers['authorization'];
  const token = authHeader && authHeader.split(' ')[1]; // Bearer TOKEN format
  
  if (!token) {
    return res.status(401).json({ success: false, message: 'Authentication required' });
  }
  
  jwt.verify(token, JWT_SECRET, (err, user) => {
    if (err) {
      return res.status(403).json({ success: false, message: 'Token expired or invalid' });
    }
    
    req.user = user;
    next();
  });
};


// Update login endpoint to use JWT
app.post('/api/login', async (req, res) => {
  try {
    const { username, password } = req.body;
    
    // Get user with hashed password
    const users = await executeQuery(
      'SELECT id, password FROM users WHERE username = ?',
      [username]
    );
    
    if (users.length === 0) {
      return res.json({ success: false, message: 'Invalid username or password' });
    }
    
    // Verify password
    const user = users[0];
    const passwordValid = await verifyPassword(password, user.password);
    
    if (passwordValid) {
      // Update last login time
      await executeQuery(
        'UPDATE users SET last_login = NOW() WHERE id = ?',
        [user.id]
      );
      
      // Generate JWT token
      const token = jwt.sign(
        { userId: user.id, username },
        JWT_SECRET,
        { expiresIn: JWT_EXPIRATION }
      );
      
      res.json({ 
        success: true,
        userId: user.id,
        token
      });
    } else {
      res.json({ success: false, message: 'Invalid username or password' });
    }
  } catch (error) {
    console.error('Login error:', error);
    res.status(500).json({ success: false, message: 'Server error' });
  }
});

// Update signup endpoint too
app.post('/api/signup', async (req, res) => {
  try {
    const { username, password } = req.body;
    
    // Check if username already exists
    const checkResults = await executeQuery(
      'SELECT COUNT(*) as count FROM users WHERE username = ?',
      [username]
    );
    
    if (checkResults[0].count > 0) {
      return res.json({ success: false, message: 'Username already taken, please choose a different one' });
    }
    
    // Hash password before storing
    const hashedPassword = await hashPassword(password);
    
    // Insert new user
    const insertResults = await executeQuery(
      'INSERT INTO users (username, password) VALUES (?, ?)',
      [username, hashedPassword]
    );
    
    // Generate JWT token for new user
    const token = jwt.sign(
      { userId: insertResults.insertId, username },
      JWT_SECRET,
      { expiresIn: JWT_EXPIRATION }
    );
    
    res.json({ 
      success: true,
      userId: insertResults.insertId,
      token
    });
  } catch (error) {
    console.error('Signup error:', error);
    res.status(500).json({ success: false, message: 'Server error' });
  }
});

// token validation endpoint
app.get('/api/validate-token', authenticateToken, (req, res) => {
  // If middleware passes, token is valid
  res.json({ valid: true, userId: req.user.userId, username: req.user.username });
});

// Create VM endpoint
app.post('/api/create-vm', authenticateToken, async (req, res) => {
  try {
    const { vmName, diskSize = 10 } = req.body;
    const userId = req.user.userId; // Use authenticated user ID from JWT

    // Validate disk size (2MB to 25MB)
    if (diskSize < 2 || diskSize > 25) {
      return res.json({ 
        success: false, 
        message: 'Disk size must be between 2MB and 25MB' 
      });
    }

    // Check if VM name already exists for this user
    const existingVM = await executeQuery(
      'SELECT COUNT(*) as count FROM vms WHERE user_id = ? AND vm_name = ?',
      [userId, vmName]
    );

    if (existingVM[0].count > 0) {
      return res.json({
        success: false,
        message: 'You already have a VM with this name. Please choose a different name.'
      });
    }

    // Create VM (linked to authenticated user)
    await executeQuery(
      'INSERT INTO vms (user_id, vm_name, disk_size) VALUES (?, ?, ?)',
      [userId, vmName, diskSize]
    );

    res.json({ success: true });
  } catch (error) {
    console.error('Create VM error:', error);
    res.status(500).json({ success: false, message: 'Server error' });
  }
});

// Get user VMs endpoint
app.get('/api/vms', authenticateToken, async (req, res) => {
  try {
    const userId = req.user.userId; // Ensure the user only accesses their own VMs

    const vms = await executeQuery(
      'SELECT id, vm_name, disk_size, status, created_at, last_active FROM vms WHERE user_id = ?',
      [userId]
    );

    res.json({ success: true, vms });
  } catch (error) {
    console.error('Get VMs error:', error);
    res.status(500).json({ success: false, message: 'Server error' });
  }
});

// Get single VM details endpoint
app.get('/api/vm/:vmId', authenticateToken, async (req, res) => {
  try {
    const vmId = req.params.vmId;
    const userId = req.user.userId; // Ensure the user only accesses their own VM

    const vms = await executeQuery(
      'SELECT id, vm_name, disk_size, status, created_at, last_active FROM vms WHERE id = ? AND user_id = ?',
      [vmId, userId]
    );

    if (vms.length === 0) {
      return res.status(404).json({ 
        success: false, 
        message: 'Virtual machine not found' 
      });
    }

    res.json({ success: true, vm: vms[0] });
  } catch (error) {
    console.error('Get VM details error:', error);
    res.status(500).json({ success: false, message: 'Server error' });
  }
});

// delete VM endpoint
app.delete('/api/vm/:vmId', authenticateToken, async (req, res) => {
  try {
    const vmId = req.params.vmId;
    const userId = req.user.userId;

    // Verify that the VM belongs to the authenticated user
    const vmResults = await executeQuery(
      'SELECT COUNT(*) as count FROM vms WHERE id = ? AND user_id = ?',
      [vmId, userId]
    );

    if (vmResults[0].count === 0) {
      return res.status(403).json({ success: false, message: 'Access denied' });
    }

    // Delete VM
    await executeQuery(
      'DELETE FROM vms WHERE id = ?',
      [vmId]
    );

    res.json({ success: true });
  } catch (error) {
    console.error('Delete VM error:', error);
    res.status(500).json({ success: false, message: 'Server error' });
  }
});


// VM control endpoint (start, stop, restart)
app.post('/api/vm/:vmId/control', authenticateToken, async (req, res) => {
  try {
    const { action } = req.body;
    const vmId = req.params.vmId;
    const userId = req.user.userId; // Get authenticated user ID

    // Verify that the VM belongs to the authenticated user
    const vmResults = await executeQuery(
      'SELECT COUNT(*) as count FROM vms WHERE id = ? AND user_id = ?',
      [vmId, userId]
    );

    if (vmResults[0].count === 0) {
      return res.status(403).json({ success: false, message: 'Access denied' });
    }

    // Update VM status based on action
    let newStatus;
    switch (action) {
      case 'start':
        newStatus = 'running';
        break;
      case 'stop':
        newStatus = 'stopped';
        break;
      case 'restart':
        newStatus = 'restarting';
        break;
      default:
        return res.json({ success: false, message: 'Invalid action' });
    }

    await executeQuery(
      'UPDATE vms SET status = ? WHERE id = ?',
      [newStatus, vmId]
    );

    res.json({ success: true, status: newStatus });
  } catch (error) {
    console.error('VM control error:', error);
    res.status(500).json({ success: false, message: 'Server error' });
  }
});


// API endpoint to generate a web terminal token
app.get('/api/vm/:vmId/web-terminal-token', authenticateToken, async (req, res) => {
  try {
    const vmId = req.params.vmId;
    const userId = req.user.userId;

    // Verify that the VM belongs to the authenticated user and is running
    const vms = await executeQuery(
      'SELECT status FROM vms WHERE id = ? AND user_id = ?',
      [vmId, userId]
    );

    if (vms.length === 0) {
      return res.status(403).json({ 
        success: false, 
        message: 'Access denied or VM not found' 
      });
    }
    
    // Remove any existing tokens for this VM
    await executeQuery(
      'DELETE FROM terminal_tokens WHERE vm_id = ?',
      [vmId]
    );

    // Generate a new token with 5 minute expiration
    const token = generateToken();
    const expiresAt = new Date(Date.now() + 5 * 60 * 1000); // 5 minutes from now
    
    // Save the token to database
    await executeQuery(
      'INSERT INTO terminal_tokens (vm_id, token, expires_at) VALUES (?, ?, ?)',
      [vmId, token, expiresAt]
    );
    
    res.json({ 
      success: true, 
      token: token,
      expiresAt: expiresAt
    });
  } catch (error) {
    console.error('Web terminal token error:', error);
    res.status(500).json({ success: false, message: 'Server error' });
  }
});

// Endpoint to validate a terminal token (will be used by the terminal service)
app.get('/api/validate-terminal-token/:token', async (req, res) => {
  try {
    const token = req.params.token;
    
    // Check if token exists and is not expired
    const tokens = await executeQuery(
      'SELECT vm_id, expires_at FROM terminal_tokens WHERE token = ? AND expires_at > NOW()',
      [token]
    );
    
    if (tokens.length === 0) {
      return res.status(403).json({ 
        success: false, 
        valid: false,
        message: 'Invalid or expired token' 
      });
    }
    
    // Return VM ID for the terminal service to connect to the right VM
    res.json({ 
      success: true, 
      valid: true,
      vmId: tokens[0].vm_id
    });
  } catch (error) {
    console.error('Token validation error:', error);
    res.status(500).json({ success: false, message: 'Server error' });
  }
});



// SSH Server API Endpoints
// 1. Endpoint to validate user and get their VMs
app.post('/api-ssh-server/validate-user', async (req, res) => {
  try {
    const { username, password } = req.body;
    
    // Get user with hashed password
    const users = await executeQuery(
      'SELECT id, password FROM users WHERE username = ?',
      [username]
    );
    
    if (users.length === 0) {
      return res.json({ success: false, message: 'Invalid username or password' });
    }
    
    // Verify password
    const user = users[0];
    const passwordValid = await verifyPassword(password, user.password);
    
    if (!passwordValid) {
      return res.json({ success: false, message: 'Invalid username or password' });
    }
    
    // Get all VMs for this user
    const vms = await executeQuery(
      'SELECT id, vm_name, status, disk_size, vm_name FROM vms WHERE user_id = ?',
      [user.id]
    );
    
    res.json({ 
      success: true, 
      userId: user.id,
      vms: vms
    });
  } catch (error) {
    console.error('SSH server validate user error:', error);
    res.status(500).json({ success: false, message: 'Server error' });
  }
});

// 2. Endpoint to validate user and specific VM
app.post('/api-ssh-server/validate-vm', async (req, res) => {
  try {
    const { username, password, vmName } = req.body;
    
    // First validate the user
    const users = await executeQuery(
      'SELECT id, password FROM users WHERE username = ?',
      [username]
    );
    
    if (users.length === 0) {
      console.log(username, password, vmName);
      console.log("SELECT id, password FROM users WHERE username = ?");
      return res.json({ success: false, message: 'Invalid username or password' });
    }
    
    // Verify password
    const user = users[0];
    const passwordValid = await verifyPassword(password, user.password);
    
    if (!passwordValid) {
      return res.json({ success: false, message: 'Invalid username or password' });
    }
    
    // Check if user has a VM with the specified name
    const vms = await executeQuery(
      'SELECT id, status, disk_size FROM vms WHERE user_id = ? AND vm_name = ?',
      [user.id, vmName]
    );
    
    if (vms.length === 0) {
      return res.json({ success: false, message: 'VM not found' });
    }
    
    res.json({ 
      success: true, 
      userId: user.id,
      vmId: vms[0].id,
      status: vms[0].status,
      diskSize: vms[0].disk_size,
      vmName: vmName
    });
  } catch (error) {
    console.error('SSH server validate VM error:', error);
    res.status(500).json({ success: false, message: 'Server error' });
  }
});

// 3. Endpoint to validate terminal token for specific user and VM
app.post('/api-ssh-server/validate-terminal-token', async (req, res) => {
  try {
    const { username, vmId, token } = req.body;
    
    // Validate the user exists
    const users = await executeQuery(
      'SELECT id FROM users WHERE username = ?',
      [username]
    );
    
    if (users.length === 0) {
      return res.json({ success: false, message: 'Invalid username' });
    }
    
    const userId = users[0].id;
    
    // Verify the VM belongs to the user and get status
    const vms = await executeQuery(
      'SELECT id, status, disk_size, vm_name FROM vms WHERE id = ? AND user_id = ?',
      [vmId, userId]
    );
    
    if (vms.length === 0) {
      return res.json({ success: false, message: 'VM not found or not owned by user' });
    }
    
    // Check if token exists and is not expired
    const tokens = await executeQuery(
      'SELECT id FROM terminal_tokens WHERE token = ? AND vm_id = ? AND expires_at > NOW()',
      [token, vmId]
    );
    
    if (tokens.length === 0) {
      return res.json({ 
        success: false, 
        valid: false,
        message: 'Invalid or expired token' 
      });
    }
    
    // Delete the token after use (single-use token)
    await executeQuery(
      'DELETE FROM terminal_tokens WHERE id = ?',
      [tokens[0].id]
    );
    
    res.json({ 
      success: true, 
      valid: true,
      vmId: vmId,
      userId: userId,
      status: vms[0].status,
      diskSize: vms[0].disk_size, 
      vmName: vms[0].vm_name
    });
  } catch (error) {
    console.error('SSH server terminal token validation error:', error);
    res.status(500).json({ success: false, message: 'Server error' });
  }
});

app.post('/api-ssh-server/update-vm-status', async (req, res) => {
  try {
    const { vmId, newStatus, secretKey } = req.body;
    
    // Verify the secret key to ensure only your SSH server can call this endpoint
    if (secretKey !== process.env.SSH_SERVER_SECRET_KEY) {
      return res.status(403).json({ 
        success: false, 
        message: 'Unauthorized access' 
      });
    }
    
    // Validate status value
    const validStatuses = ['running', 'stopped'];
    if (!validStatuses.includes(newStatus)) {
      return res.status(400).json({
        success: false,
        message: 'Invalid status value'
      });
    }
    
    // Update VM status
    const result = await executeQuery(
      'UPDATE vms SET status = ?, last_active = NOW() WHERE id = ?',
      [newStatus, vmId]
    );

    
    if (result.affectedRows === 0) {
      return res.status(404).json({
        success: false,
        message: 'VM not found'
      });
    }
    
    res.json({
      success: true,
      message: 'VM status updated successfully'
    });
  } catch (error) {
    console.error('SSH server update VM status error:', error);
    res.status(500).json({ success: false, message: 'Server error' });
  }
});

// Serve index.html for the root URL
app.get('*', (req, res) => {
  res.sendFile(path.join(__dirname, './remotekvm-frontend/build', 'index.html'));
});

const options = {
  key: fs.readFileSync('/etc/letsencrypt/live/remotekvm.online/privkey.pem'),
  cert: fs.readFileSync('/etc/letsencrypt/live/remotekvm.online/fullchain.pem'),
};

https.createServer(options, app).listen(443, () => {
    console.log('HTTPS server running on https://localhost:443');
});

// Graceful shutdown to close database connections
process.on('SIGINT', async () => {
  try {
    await pool.end();
    console.log('Database connection pool closed');
    process.exit(0);
  } catch (error) {
    console.error('Error closing database connections:', error);
    process.exit(1);
  }
});
