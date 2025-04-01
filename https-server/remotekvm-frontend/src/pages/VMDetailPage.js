import React, { useState, useEffect, useRef } from 'react';
import { useParams, useNavigate } from 'react-router-dom';
import TopMenu from '../components/TopMenu';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { 
  faChevronLeft, faHdd, faTrash, 
  faCopy, faLaptop, faExclamationTriangle,
  faCode, faCalendarAlt, faClock, faCircle, faFileLines, faCheck
} from '@fortawesome/free-solid-svg-icons';
import { Terminal } from 'xterm';
import 'xterm/css/xterm.css';
import styles from './VMDetailPage.module.css';

function VMDetailPage() {
  const { id } = useParams();
  const [vm, setVm] = useState(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
  const [isVisible, setIsVisible] = useState(false);
  const [connectionMethod, setConnectionMethod] = useState(null); // null, 'ssh', 'details', 'terminal'
  const [terminalConnected, setTerminalConnected] = useState(false);
  const navigate = useNavigate();
  const terminalRef = useRef(null);
  const terminalInstance = useRef(null);
  const socketRef = useRef(null);
  const [copiedSSH, setCopiedSSH] = useState(false);
  const [copiedHostname, setCopiedHostname] = useState(false);
  const [copiedPort, setCopiedPort] = useState(false);
  const [copiedUsername, setCopiedUsername] = useState(false);
  const statusPollingRef = useRef(null);
  const [showDeleteConfirm, setShowDeleteConfirm] = useState(false);
  const [isDeleting, setIsDeleting] = useState(false);



    // Status animation states
    const [statusAnimation, setStatusAnimation] = useState(null);
    const [statusNotification, setStatusNotification] = useState({ visible: false, status: null });
    const previousStatusRef = useRef(null);
  

  const handleCopySSH = () => {
    if (!vm) return;

    const sshCommand = `ssh ${localStorage.getItem("username") + "#" + vm.vm_name}@remotekvm.online -p 2222`;
        
    navigator.clipboard.writeText(sshCommand)
      .then(() => {
        console.log("Command copied successfully");
        setCopiedSSH(true);
        
        // Reset after 2 seconds
        setTimeout(() => {
          setCopiedSSH(false);
        }, 2000);
      })
      .catch(err => {
        console.error('Failed to copy text: ', err);
        alert("Failed to copy to clipboard. Please try again.");
      });
  };


  const handleConnectionMethodChange = (method) => {
    // First fade out the current method if one is selected
    if (connectionMethod !== null) {
      // Create a temporary div to hold the old content during transition
      const contentElement = document.querySelector(`.${styles["connection-method-content"]}`);
      if (contentElement) {
        //contentElement.style.animation = 'fadeSlideOut 0.2s ease forwards';
        
        // After fade out animation completes, change the method
        setTimeout(() => {
          setConnectionMethod(method);
        }, 200);
      } else {
        setConnectionMethod(method);
      }
    } else {
      // No current method, just set the new one immediately
      setConnectionMethod(method);
    }
  };

  useEffect(() => {
    if (connectionMethod === 'terminal') {
      // Short delay to ensure DOM is ready
      setTimeout(() => {
        initTerminal();
      }, 100);
    }
  }, [connectionMethod]);

  const handleBackToMethods = () => {
    const contentElement = document.querySelector(`.${styles["connection-method-content"]}`);
    
    // Close the WebSocket connection if it exists and we're in terminal mode
    if (connectionMethod === 'terminal' && socketRef.current) {
      console.log('Closing WebSocket connection when leaving terminal view');
      socketRef.current.close();
      socketRef.current = null;
      setTerminalConnected(false);
    }

    if (contentElement) {
      // Start fading out the current content
      contentElement.style.animation = 'fadeSlideOut 0.3s ease forwards';
      setTimeout(() => {
        setConnectionMethod(null);
      }, 300); // Match this to the fadeSlideOut animation duration
    } else {
      // No content element found, just switch directly
      setConnectionMethod(null);
    }
  };

  // Load xterm.js dynamically
  useEffect(() => {
    // No dynamic loading needed anymore
    return () => {
      // Clean up terminal if needed
      if (terminalInstance.current) {
        terminalInstance.current.dispose();
      }
    };
  }, []);
  
  

  useEffect(() => {
    const timer = setTimeout(() => {
      setIsVisible(true);
    }, 100);
    
    // Validate token first
    validateToken();
    
    return () => {
      clearTimeout(timer);
      // Clean up terminal connection if exists
      if (socketRef.current) {
        console.log('Closing WebSocket connection on component unmount');
        socketRef.current.close();
        socketRef.current = null;
        setTerminalConnected(false);
      }
      
      // Clean up terminal instance if exists
      if (terminalInstance.current) {
        terminalInstance.current.dispose();
        terminalInstance.current = null;
      }
    };
  }, []);

  // Validate token before allowing access to this page
  const validateToken = async () => {
    const token = localStorage.getItem('token');
    if (!token) {
      navigate('/login');
      return;
    }

    try {
      const response = await fetch('/api/validate-token', {
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });

      const data = await response.json();
      
      if (!response.ok || !data.valid) {
        localStorage.removeItem('token');
        localStorage.removeItem('userId');
        localStorage.removeItem('username');
        localStorage.removeItem('isLoggedIn');
        navigate('/login');
      } else {
        // Token is valid, fetch VM details
        fetchVMDetails();
      }
    } catch (err) {
      console.error('Token validation error:', err);
      localStorage.removeItem('token');
      localStorage.removeItem('userId');
      localStorage.removeItem('username');
      localStorage.removeItem('isLoggedIn');
      navigate('/login');
    }
  };

  // When VM data is first loaded, set the initial previous status
  useEffect(() => {
    if (vm && previousStatusRef.current === null) {
      previousStatusRef.current = vm.status;
    }
  }, [vm]);

  // Implement periodic status polling
  useEffect(() => {
    // Start polling when VM data is loaded
    if (vm) {
      // Immediately start polling for status updates
      statusPollingRef.current = setInterval(() => {
        fetchVMDetails(false); // Pass false to indicate this is a status update, not initial load
      }, 10000); // Poll every 10 seconds
    }

    // Cleanup on unmount
    return () => {
      if (statusPollingRef.current) {
        clearInterval(statusPollingRef.current);
      }
    };
  }, [vm?.id]); // Only re-run when VM ID changes  
  

  const fetchVMDetails = async (isInitialLoad = true) => {
    try {
      // Only show loading indicator on initial load
      if (isInitialLoad) {
        setLoading(true);
      }
      
      // Get auth token
      const token = localStorage.getItem('token');
      if (!token) {
        throw new Error('Authentication required');
      }
      
      // Fetch VM details with proper authentication
      const response = await fetch(`/api/vm/${id}`, {
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });
      
      if (!response.ok) {
        // Handle error status codes
        if (response.status === 403) {
          throw new Error('You do not have permission to access this VM');
        } else if (response.status === 404) {
          throw new Error('Virtual machine not found');
        } else {
          throw new Error('Failed to load virtual machine details');
        }
      }
      
      const data = await response.json();
      
      if (data.success && data.vm) {
        // Set initial previousStatus if first time
        if (!isInitialLoad && previousStatusRef.current !== null && data.vm.status !== previousStatusRef.current) {
          handleStatusChange(data.vm.status);
        }
        
        // Update previous status reference before updating the state
        previousStatusRef.current = data.vm.status;
        
        // Update VM state
        setVm(data.vm);
      } else {
        throw new Error(data.message || 'Failed to load virtual machine details');
      }
      
      // Only update loading state on initial load
      if (isInitialLoad) {
        setLoading(false);
      }
    } catch (err) {
      console.error('Error fetching VM details:', err);
      // Only show errors and update loading state on initial load
      if (isInitialLoad) {
        setError(err.message || 'Failed to load virtual machine details.');
        setLoading(false);
      }
    }
  };

   // Handler for status changes
   const handleStatusChange = (newStatus) => {
    const statusLower = newStatus.toLowerCase();
    
    // Set the animation based on the new status
    if (statusLower === 'running' || statusLower === 'stopped') {
      // Get the status indicator
      const statusIndicator = document.querySelector(`.${styles.statusIndicator}`);
      
      // Set the page-wide animation
      setStatusAnimation(statusLower);
      
      // Animate the status indicator
      if (statusIndicator) {
        statusIndicator.classList.remove(styles["animate-running"], styles["animate-stopped"]);
        void statusIndicator.offsetWidth;
        
        if (statusLower === 'running') {
          statusIndicator.classList.add(styles["animate-running"]);
        } else if (statusLower === 'stopped') {
          statusIndicator.classList.add(styles["animate-stopped"]);
        }
      }
      
      // Reset the animation after it completes
      setTimeout(() => {
        setStatusAnimation(null);
      }, 4000); // Match this to the animation duration
      
    }
  };




  const handleDeleteVM = async () => {
    setShowDeleteConfirm(true);
  };
  
  const confirmDeleteVM = async () => {
    try {
      setIsDeleting(true);
      
      // Get auth token
      const token = localStorage.getItem('token');
      if (!token) {
        throw new Error('Authentication required');
      }
      
      // Call delete VM endpoint with proper authentication
      const response = await fetch(`/api/vm/${id}`, {
        method: 'DELETE',
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });
      
      if (!response.ok) {
        // Handle error status codes
        if (response.status === 403) {
          throw new Error('You do not have permission to delete this VM');
        } else {
          throw new Error('Failed to delete VM');
        }
      }
      
      const data = await response.json();
      
      if (data.success) {
        // Navigate back to dashboard on successful deletion
        navigate('/dashboard');
      } else {
        throw new Error(data.message || 'Failed to delete VM');
      }
    } catch (err) {
      console.error('Error deleting VM:', err);
      alert(err.message || 'Failed to delete VM. Please try again.');
    } finally {
      setIsDeleting(false);
      setShowDeleteConfirm(false);
    }
  };

    // Helper function to format date as DD/MM/YYYY HH:MM
    const formatDate = (dateString) => {
      if (!dateString) return 'Never';
      
      const date = new Date(dateString);
      
      // Format as DD/MM/YYYY HH:MM
      const day = date.getDate().toString().padStart(2, '0');
      const month = (date.getMonth() + 1).toString().padStart(2, '0');
      const year = date.getFullYear();
      const hours = date.getHours().toString().padStart(2, '0');
      const minutes = date.getMinutes().toString().padStart(2, '0');
      
      return `${day}/${month}/${year} ${hours}:${minutes}`;
    };

    const formatLastActive = (dateString) => {
      if (!dateString) return 'Never';
      
      // Check if the VM is currently active
      if (vm && vm.status && vm.status.toLowerCase() === 'running') {
        return 'Currently Active';
      }
      
      return formatDate(dateString);
    };

  // Helper function to get status color
  const getStatusColor = (status) => {
    switch(status?.toLowerCase()) {
      case 'running': return '#09bc8a'; // Light green
      case 'stopped': return '#ff5f56'; // Red
      default: return '#508991'; // Blue
    }
  };

  // Copy connection detail to clipboard
  const copyToClipboard = (text, type) => {
    navigator.clipboard.writeText(text)
      .then(() => {
        console.log(`${type} copied successfully`);
        
        // Set the appropriate state based on what was copied
        switch(type) {
          case 'Hostname':
            setCopiedHostname(true);
            setTimeout(() => setCopiedHostname(false), 2000);
            break;
          case 'Port':
            setCopiedPort(true);
            setTimeout(() => setCopiedPort(false), 2000);
            break;
          case 'Username':
            setCopiedUsername(true);
            setTimeout(() => setCopiedUsername(false), 2000);
            break;
          default:
            break;
        }
      })
      .catch(err => {
        console.error('Failed to copy text: ', err);
      });
  };

  const initTerminal = async () => {
    console.log('Terminal initialization starting');
    console.log('Terminal ref exists:', !!terminalRef.current);
    
    // Make sure the terminal ref exists  
    if (!terminalRef.current) {
      console.error('Terminal reference is not available');
      return;
    }
    
    // Clear previous terminal if exists
    if (terminalInstance.current) {
      terminalInstance.current.dispose();
    }
    
    try {
      console.log('Creating new terminal instance');
      // Create new terminal using the imported Terminal class
      const terminal = new Terminal({
        cursorBlink: true,
        theme: {
          background: '#1e1e1e',
          foreground: '#f0f0f0'
        },
        fontSize: 14,
        fontFamily: 'Menlo, Monaco, "Courier New", monospace',
        cols: 120,
      });
      
      console.log('Opening terminal in container');
      terminal.open(terminalRef.current);
      terminalInstance.current = terminal;
      
      // Write initial message to terminal
      terminal.writeln('Initializing terminal...');
      
      // Connect to WebSocket for terminal
      connectTerminal();
    } catch (err) {
      console.error('Error initializing terminal:', err);
      
      // Create a fallback display on error
      if (terminalRef.current) {
        terminalRef.current.innerHTML = `
          <div style="background-color: #1e1e1e; color: #f0f0f0; padding: 10px; font-family: monospace; height: 300px; overflow-y: auto;">
            <p>Error initializing terminal: ${err.message}</p>
            <p>Please refresh the page and try again.</p>
          </div>
        `;
      }
    }
  };
  

  const connectTerminal = async () => {
    if (!terminalInstance.current) return;
    
    const token = localStorage.getItem('token');
    if (!token) {
      terminalInstance.current.writeln('Error: Authentication required');
      return;
    }
    
    try {             
      // Request terminal token
      const tokenResponse = await fetch(`/api/vm/${id}/web-terminal-token`, {
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });
      
      // Check if the response is JSON
      const contentType = tokenResponse.headers.get("content-type");
      if (!contentType || !contentType.includes("application/json")) {
        throw new Error(`Server returned non-JSON response (${contentType}). Please check the server configuration.`);
      }
      
      if (!tokenResponse.ok) {
        const errorData = await tokenResponse.json();
        throw new Error(errorData.message || `Failed to get terminal access (Status: ${tokenResponse.status})`);
      }
      
      const tokenData = await tokenResponse.json();
      
      if (!tokenData.success || !tokenData.token) {
        throw new Error('Invalid terminal token response');
      }
      
      const terminalToken = tokenData.token;
      
      // Close existing socket if any
      if (socketRef.current) {
        socketRef.current.close();
      }
  
      // Connect to WebSocket server with the terminal token
      const socket = new WebSocket("wss://remotekvm.online:2000");
      socketRef.current = socket;
      
      socket.onopen = () => {         
        // Send authentication data with terminal token
        socket.send(JSON.stringify({ 
          type: 'auth', 
          terminalToken: terminalToken,
          vmId: id,
          username: localStorage.getItem('username')
        }));
        
        // Send terminal size
        const dimensions = { cols: terminalInstance.current.cols, rows: terminalInstance.current.rows };
        socket.send(JSON.stringify({ type: 'resize', data: dimensions }));
        
        setTerminalConnected(true);
      };
      
      socket.onmessage = (event) => {
        try {
          // Try to parse as JSON first (for control messages)
          const data = JSON.parse(event.data);
          if (data.type === 'output') {
            terminalInstance.current.write(data.data);
          } else if (data.type === 'error') {
            terminalInstance.current.writeln(`\r\nError: ${data.message}`);
          } else if (data.type === 'connected') {
            terminalInstance.current.writeln('\r\nConnected to terminal.');
          } else if (data.type === 'disconnected') {
            terminalInstance.current.writeln('\r\nDisconnected from terminal.');
          }
        } catch (err) {
          // If not JSON, write directly to terminal
          // This is the raw terminal output
          terminalInstance.current.write(event.data);
        }
      };
      
      socket.onclose = (event) => {
        let message = '\r\nConnection closed';
        if (event.code !== 1000) {
          message += ` (Code: ${event.code})`;
        }
        terminalInstance.current.writeln(message);
        setTerminalConnected(false);
      };
      
      socket.onerror = (error) => {
        console.error('WebSocket error:', error);
        terminalInstance.current.writeln('\r\nError: Connection failed');
        setTerminalConnected(false);
      };
      
      // Handle terminal input
      terminalInstance.current.onData(data => {
        if (socket && socket.readyState === WebSocket.OPEN) {
          // Send raw data for simpler processing
          socket.send(data);
        }
      });
    } catch (err) {
      console.error('Terminal connection error:', err);
      terminalInstance.current.writeln(`\r\nError: ${err.message}`);
      setTerminalConnected(false);
    }
  };
  

    return (
    <>
      <TopMenu />

      {/* Full-page status animation overlay */}
      {statusAnimation && (
        <div className={`${styles['status-animation-overlay']} ${styles[`status-animation-${statusAnimation}`]}`}></div>
      )}

      <div className={styles["vm-detail-page"]}>
        <div className={`${styles['vm-detail-container']} ${isVisible ? styles['visible'] : ''}`}> 
           <div className={styles["back-link"]} onClick={() => navigate('/dashboard')}>
            <FontAwesomeIcon icon={faChevronLeft} /> Back to Dashboard
          </div>
          
          {loading ? (
            <div className={styles["vm-detail-loading"]}>
              <div className={styles["loading-spinner"]}></div>
              <p>Loading virtual machine details...</p>
            </div>
          ) : error ? (
            <div className={styles["vm-detail-error"]}>
              <FontAwesomeIcon icon={faExclamationTriangle} className={styles["error-icon"]} />
              <p>{error}</p>
              <button onClick={validateToken}>Try Again</button>
              <button onClick={() => navigate('/dashboard')}>Back to Dashboard</button>
            </div>
          ) : vm ? (
            <>
              <div className={`${styles["vm-detail-header"]} ${styles["simplified-header"]}`}>
                <div className={styles["vm-name-status"]}>
                  <h2>{vm.vm_name}</h2>
                  <span 
                    className={styles.statusIndicator}
                    style={{ backgroundColor: getStatusColor(vm.status) }}
                  >
                    <FontAwesomeIcon icon={faCircle} className={styles.statusIcon} />
                    {vm.status}
                  </span>
                </div>
                
                <div className={styles["vm-simple-info"]}>
                  <span className={styles["spec-span-text"]}><FontAwesomeIcon icon={faHdd} /> Storage: {vm.disk_size} MB</span>
                  <span className={styles["spec-span-text"]}><FontAwesomeIcon icon={faClock} /> Last Active: {formatLastActive(vm.last_active)}</span>
                  <span className={styles["spec-span-text"]}><FontAwesomeIcon icon={faCalendarAlt} /> Created: {formatDate(vm.created_at)}</span>
                </div>
              </div>
  
              <div className={styles["vm-detail-body"]}>
                <div className={`${styles["vm-detail-card"]} ${styles["vm-connection"]}`}>
                  <div className={styles["vm-detail-card-header"]}>
                    <h3>
                      Connect
                      {connectionMethod === 'ssh' && (
                        <>
                          <span className={styles["connection-method-title-separator"]}>-</span>
                          <span>SSH Command</span>
                          <FontAwesomeIcon icon={faCode} />
                        </>
                      )}
                      {connectionMethod === 'details' && (
                        <>
                          <span className={styles["connection-method-title-separator"]}>-</span>
                          <span>Connection Details</span>
                          <FontAwesomeIcon icon={faFileLines} />
                        </>
                      )}
                      {connectionMethod === 'terminal' && (
                        <>
                          <span className={styles["connection-method-title-separator"]}>-</span>
                          <span>Web Terminal</span>
                          <FontAwesomeIcon icon={faLaptop} />
                        </>
                      )}
                    </h3>
                    {connectionMethod !== null && (
                      <button 
                        className={styles["connection-back-link"]} 
                        onClick={handleBackToMethods}
                        aria-label="Back to connection methods"
                      >
                        <FontAwesomeIcon icon={faChevronLeft} /> Back to connection methods
                      </button>
                    )}
                  </div>
                  
                  {connectionMethod === null ? (
                    <div className={`${styles["connection-options-selector"]} ${styles["connection-method-content"]}`} id="connection-options">
                    <p className={styles["connection-intro"]}>
                      Select a connection method to access your virtual machine. 
                      The VM will start automatically when you connect and stop when you disconnect.
                    </p>
                    <div className={styles["connection-methods"]}>
                      <div className={styles["connection-method-card"]} onClick={() => handleConnectionMethodChange('terminal')}>
                        <FontAwesomeIcon icon={faLaptop} className={styles["connection-method-icon"]} />
                        <h4>Web Terminal</h4>
                        <p>Connect directly from your browser without additional software</p>
                        <span className={styles["connection-method-badge"]}>Fastest Option</span>
                      </div>
                      <div className={styles["connection-method-card"]} onClick={() => handleConnectionMethodChange('ssh')}>
                        <FontAwesomeIcon icon={faCode} className={styles["connection-method-icon"]} />
                        <h4>SSH Command</h4>
                        <p>Copy a ready-to-use SSH command for terminal access</p>
                        <span className={styles["connection-method-badge"]}>For Terminal Users</span>
                      </div>
                      <div className={styles["connection-method-card"]} onClick={() => handleConnectionMethodChange('details')}>
                        <FontAwesomeIcon icon={faFileLines} className={styles["connection-method-icon"]} />
                        <h4>Connection Details</h4>
                        <p>Get host, port and username for SSH clients like PuTTY</p>
                        <span className={styles["connection-method-badge"]}>For PuTTY Users</span>
                      </div>
                    </div>
                  </div>
                  ) : (
                    <div className={styles["connection-method-transition"]}>
                      {connectionMethod === 'ssh' && (
                        <div className={`${styles["connection-ssh-section"]} ${styles["connection-method-content"]}`}>
                          <p className={styles["paragrath-dark"]}>Use this command in your terminal to connect to your virtual machine:</p>
                          <div className={styles["connection-command"]}>
                            <code>ssh {localStorage.getItem("username") + "#" + vm.vm_name}@remotekvm.online -p 2222</code>
                            <button 
                              className={`${styles["copy-btn"]} ${copiedSSH ? styles["copied"] : ""}`} 
                              onClick={handleCopySSH} 
                              aria-label="Copy SSH command"
                            >
                              <FontAwesomeIcon icon={copiedSSH ? faCheck : faCopy} />
                              <span className={styles["copy-tooltip"]}>Copied!</span>
                            </button>
                          </div>
                        </div>
                      )}
                      
                      {connectionMethod === 'details' && (
                        <div className={`${styles["connection-details-section"]} ${styles["connection-method-content"]}`}>
                          <p className={styles["paragrath-dark"]}>Use these details to connect via PuTTY or other SSH clients:</p>
                          <div className={styles["connection-details-list"]}>
                            <div className={styles["connection-detail-row"]}>
                              <strong>Hostname:</strong>
                              <div className={styles["detail-value-container"]}>
                                <span className={styles["detail-value"]}>{'remotekvm.online'}</span>
                                <button 
                                  className={`${styles["copy-detail-btn"]} ${copiedHostname ? styles["copied"] : ""}`} 
                                  onClick={() => copyToClipboard('remotekvm.online', 'Hostname')}
                                  aria-label="Copy hostname"
                                >
                                  <FontAwesomeIcon icon={copiedHostname ? faCheck : faCopy} />
                                  <span className={styles["copy-tooltip"]}>Copied!</span>
                                </button>
                              </div>
                            </div>
                            
                            <div className={styles["connection-detail-row"]}>
                              <strong>Port:</strong>
                              <div className={styles["detail-value-container"]}>
                                <span className={styles["detail-value"]}>{2222}</span>
                                <button 
                                  className={`${styles["copy-detail-btn"]} ${copiedPort ? styles["copied"] : ""}`} 
                                  onClick={() => copyToClipboard('2222', 'Port')}
                                  aria-label="Copy port"
                                >
                                  <FontAwesomeIcon icon={copiedPort ? faCheck : faCopy} />
                                  <span className={styles["copy-tooltip"]}>Copied!</span>
                                </button>
                              </div>
                            </div>
                            
                            <div className={styles["connection-detail-row"]}>
                              <strong>Username:</strong>
                              <div className={styles["detail-value-container"]}>
                                <span className={styles["detail-value"]}>{localStorage.getItem("username") + "#" + vm.vm_name}</span>
                                <button 
                                  className={`${styles["copy-detail-btn"]} ${copiedUsername ? styles["copied"] : ""}`} 
                                  onClick={() => copyToClipboard(localStorage.getItem("username") + "#" + vm.vm_name, 'Username')}
                                  aria-label="Copy username"
                                >
                                  <FontAwesomeIcon icon={copiedUsername ? faCheck : faCopy} />
                                  <span className={styles["copy-tooltip"]}>Copied!</span>
                                </button>
                              </div>
                            </div>
                          </div>
                        </div>
                      )}
                      
                      {connectionMethod === 'terminal' && (
                        <div className={`${styles["connection-terminal-section"]} ${styles["connection-method-content"]}`}>
                          <p className={styles["paragrath-dark"]}>Connect directly to your VM through this browser-based terminal:</p>
                          <div ref={terminalRef} className={styles["terminal-container"]}></div>
                        </div>
                      )}
                    </div>
                  )}
                </div>
                
                <div className={`${styles["vm-detail-card"]} ${styles["vm-danger"]}`}>
                  <h3>Danger Zone</h3>
                  <p>Permanently delete this virtual machine and all associated data.</p>
                  <button className={styles["delete-vm-btn"]} onClick={handleDeleteVM}>
                    <FontAwesomeIcon icon={faTrash} /> Delete VM
                  </button>
                </div>
              </div>
            </>
          ) : (
            <div className={styles["vm-detail-error"]}>
              <FontAwesomeIcon icon={faExclamationTriangle} className={styles["error-icon"]} />
              <p>VM not found</p>
              <button onClick={() => navigate('/dashboard')}>Back to Dashboard</button>
            </div>
          )}
        </div>
      </div>
        {showDeleteConfirm && (
          <div className={styles.modalOverlay}>
            <div className={styles.deleteConfirmModal}>
              <div className={styles.deleteConfirmHeader}>
                <FontAwesomeIcon icon={faExclamationTriangle} className={styles.deleteWarningIcon} />
                <h3>Delete Virtual Machine</h3>
              </div>
              <div className={styles.deleteConfirmBody}>
                <p>Are you sure you want to permanently delete <strong>{vm?.vm_name}</strong>?</p>
                <p className={styles.deleteWarningText}>This action cannot be undone and all data will be lost.</p>
              </div>
              <div className={styles.deleteConfirmActions}>
                <button 
                  className={styles.cancelDeleteBtn} 
                  onClick={() => setShowDeleteConfirm(false)}
                  disabled={isDeleting}
                >
                  Cancel
                </button>
                <button 
                  className={styles.confirmDeleteBtn} 
                  onClick={confirmDeleteVM}
                  disabled={isDeleting}
                >
                  {isDeleting ? (
                    <>
                      <div className={styles.deleteSpinner}></div>
                      Deleting...
                    </>
                  ) : (
                    <>
                      <FontAwesomeIcon icon={faTrash} />
                      Delete VM
                    </>
                  )}
                </button>
              </div>
            </div>
          </div>
        )}
    </>
  );
}

export default VMDetailPage;