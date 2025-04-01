const https = require('https');
const fs = require('fs');
const WebSocket = require('ws');
const { Client } = require('ssh2');

// Create the HTTPS server with SSL certificates
const server = https.createServer({
    key: fs.readFileSync('/etc/letsencrypt/live/remotekvm.online/privkey.pem'),
    cert: fs.readFileSync('/etc/letsencrypt/live/remotekvm.online/cert.pem')
});

// Create WebSocket server
const wss = new WebSocket.Server({ server });

wss.on('connection', function connection(ws) {
    console.log("WebSocket client connected");
    
    // Variable to store the SSH client
    let ssh = null;
    let authenticated = false;
    let authData = {};
    let terminalDimensions = { cols: 80, rows: 24 }; // Default dimensions
    
    // Set up a ping interval to detect disconnections
    const pingInterval = setInterval(() => {
        if (ws.readyState === WebSocket.OPEN) {
            ws.ping();
        }
    }, 30000);
    
    // Handle WebSocket close event
    ws.on('close', () => {
        console.log("WebSocket connection closed");
        clearInterval(pingInterval);
        
        // Close the SSH connection if it exists
        if (ssh) {
            console.log("Closing SSH connection");
            ssh.end();
            ssh = null;
        }
    });
    
    // Handle WebSocket errors
    ws.on('error', (err) => {
        console.error("WebSocket error:", err);
        clearInterval(pingInterval);
        
        // Close the SSH connection if it exists
        if (ssh) {
            ssh.end();
            ssh = null;
        }
    });
    
    // Handle incoming messages from the WebSocket
    ws.on('message', (message) => {
        try {
            // Try to parse as JSON first
            const data = JSON.parse(message);
            
            // Handle authentication
            if (data.type === 'auth') {
                authData = {
                    username: data.username,
                    terminalToken: data.terminalToken,
                    vmId: data.vmId
                };
                
                // Now that we have auth data, initialize SSH connection
                initializeSSHConnection();
                authenticated = true;
            } 
            // Handle terminal resize
            else if (data.type === 'resize' && data.data) {
                terminalDimensions = data.data;
                // If SSH stream exists, resize it
                if (ssh && ssh.sshStream) {
                    ssh.sshStream.setWindow(terminalDimensions.rows, terminalDimensions.cols, 0, 0);
                }
            }
            // Handle JSON-formatted terminal input
            else if (authenticated && ssh && ssh.sshStream && data.type === 'input') {
                if (ssh.sshStream.writable) {
                    ssh.sshStream.write(data.data);
                }
            }
        } catch (err) {
            // If not JSON, assume it's raw terminal input
            if (authenticated && ssh && ssh.sshStream && ssh.sshStream.writable) {
                ssh.sshStream.write(message.toString());
            }
        }
    });
    
    function initializeSSHConnection() {
        // Create SSH connection
        ssh = new Client();
        
        ssh.on('ready', () => {
            console.log("SSH connection established");
            ws.send(JSON.stringify({ type: 'connected', success: true }));
            
            ssh.shell({ term: 'xterm-color', rows: terminalDimensions.rows, cols: terminalDimensions.cols }, (err, stream) => {
                if (err) {
                    console.error("SSH shell error:", err);
                    ws.send(JSON.stringify({ 
                        type: 'error', 
                        message: 'Error starting shell: ' + err.message 
                    }));
                    return ws.close();
                }
                
                // Store the stream for later use (e.g., resizing)
                ssh.sshStream = stream;
                
                // Handle stream data - send raw data for terminal to display directly
                stream.on('data', (data) => {
                    if (ws.readyState === WebSocket.OPEN) {
                        ws.send(JSON.stringify({ type: 'output', data: data.toString() }));
                    }
                });
                
                // Handle stream close
                stream.on('close', () => {
                    console.log("SSH stream closed");
                    if (ws.readyState === WebSocket.OPEN) {
                        ws.send(JSON.stringify({ type: 'disconnected' }));
                        ws.close();
                    }
                });
                
                // Handle stream errors
                stream.on('error', (err) => {
                    console.error("SSH stream error:", err);
                    if (ws.readyState === WebSocket.OPEN) {
                        ws.send(JSON.stringify({ 
                            type: 'error', 
                            message: 'SSH stream error: ' + err.message 
                        }));
                        ws.close();
                    }
                });
            });
        });
        
        ssh.on('error', (err) => {
            console.error("SSH connection error:", err);
            if (ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({ 
                    type: 'error', 
                    message: 'SSH connection error: ' + err.message 
                }));
                ws.close();
            }
        });
        
        // Connect to the SSH server with the formatted username
        ssh.connect({
            host: 'localhost',
            port: 2222,
            // Format the username as required: "{username}#vmId#token"
            username: `${authData.username}#${authData.vmId}#${authData.terminalToken}`,
            password: 'webterminal',
            keepaliveInterval: 30000,
            keepaliveCountMax: 3
        });
    }
});

// Start the server
server.listen(2000, () => {
    console.log('Server running on https://remotekvm.online:2000');
});