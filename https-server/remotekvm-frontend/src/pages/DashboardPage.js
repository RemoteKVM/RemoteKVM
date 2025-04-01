import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { 
  faPlus, faServer, faChevronRight, faCircle, 
  faClock, faHdd, faCalendarAlt
} from '@fortawesome/free-solid-svg-icons';
import TopMenu from '../components/TopMenu';
import styles from './DashboardPage.module.css';

function DashboardPage() {
  const [isVisible, setIsVisible] = useState(false);
  const [vms, setVms] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
  const navigate = useNavigate();

  useEffect(() => {
    const timer = setTimeout(() => {
      setIsVisible(true);
    }, 100);
    
    // Fetch VMs once the token is validated
    fetchUserVMs();
    
    return () => {
      clearTimeout(timer);
    };
  }, []);

  useEffect(() => {
    // Check if user is logged in and has valid token
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
          localStorage.removeItem('isLoggedIn');
          navigate('/login');
        }
      } catch (err) {
        console.error('Token validation error:', err);
        localStorage.removeItem('token');
        localStorage.removeItem('isLoggedIn');
        navigate('/login');
      }
    };
  
    validateToken();
  }, [navigate]);

  const fetchUserVMs = async () => {
    try {
      setLoading(true);
      
      // Get auth token from localStorage
      const token = localStorage.getItem('token');
      
      if (!token) {
        throw new Error('Authentication required');
      }
      
      // Fetch VMs from backend API with proper authentication
      const response = await fetch('/api/vms', {
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });
      
      if (!response.ok) {
        throw new Error('Failed to fetch VMs');
      }
      
      const data = await response.json();
      
      if (data.success && data.vms) {
        setVms(data.vms);
        setLoading(false);
      } else {
        throw new Error(data.message || 'Failed to load virtual machines');
      }
    } catch (err) {
      console.error('Error fetching VMs:', err);
      setError('Failed to load virtual machines. Please try again later.');
      setLoading(false);
    }
  };

  const handleVMClick = (vmId) => {
    navigate(`/vm/${vmId}`);
  };

  const handleCreateVM = () => {
    navigate('/create-vm');
  };

  // Helper function to format date
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

  const formatLastActive = (dateString, status) => {
    if (!dateString) return 'Never';
    
    // Check if the VM is currently active
    if (status.toLowerCase() === 'running') {
      return 'Currently Active';
    }
    
    return formatDate(dateString);
  };

  // Helper function to get status color
  const getStatusColor = (status) => {
    switch (status.toLowerCase()) {
      case 'running':
        return '#09bc8a';
      case 'stopped':
        return '#ff5f56';
      case 'restarting':
        return '#ffbd2e';
      default:
        return '#888';
    }
  };

  return (
    <div className={styles.pageWrapper}>
      <div className={styles.topMenuWrapper}>
        <TopMenu />
      </div>
      <div className={styles.page}>
        <div className={`${styles.container} ${isVisible ? styles.visible : ''}`}>
          <div className={styles.header}>
            <h2>My Virtual Machines</h2>
            <button className={styles.createVmBtn} onClick={handleCreateVM}>
              <FontAwesomeIcon icon={faPlus} />
              Create VM
            </button>
          </div>
          
          {loading ? (
            <div className={styles.loadingContainer}>
              <div className={styles.loadingSpinner}></div>
              <p>Loading your virtual machines...</p>
            </div>
          ) : error ? (
            <div className={styles.errorMessage}>
              <p>{error}</p>
              <button onClick={fetchUserVMs}>Retry</button>
            </div>
          ) : vms.length === 0 ? (
            <div className={styles.noVms}>
              <FontAwesomeIcon icon={faServer} className={styles.noVmsIcon} />
              <h3>No Virtual Machines Found</h3>
              <p>Get started by creating your first virtual machine!</p>
              <button className={styles.createVmBtn} onClick={handleCreateVM}>
                <FontAwesomeIcon icon={faPlus} /> Create VM
              </button>
            </div>
          ) : (
            <div className={styles.vmList}>
              {vms.map((vm) => (
                <div 
                  key={vm.id} 
                  className={styles.vmCard}
                  onClick={() => handleVMClick(vm.id)}
                >
                  <div className={styles.vmCardHeader}>
                    <h3 className={styles.vmHeader}>{vm.vm_name}</h3>
                    <span 
                      className={styles.statusIndicator}
                      style={{ backgroundColor: getStatusColor(vm.status) }}
                    >
                      <FontAwesomeIcon icon={faCircle} className={styles.statusIcon} />
                      {vm.status}
                    </span>
                  </div>
                  
                  <div className={styles.vmSpecs}>
                    <div className={styles.vmSpec}>
                      <FontAwesomeIcon icon={faHdd} className={styles.specIcon} />
                      <span>{vm.disk_size} MB Storage</span>
                    </div>
                    <div className={styles.vmSpec}>
                      <FontAwesomeIcon icon={faClock} className={styles.specIcon} />
                      <span>Last active: {formatLastActive(vm.last_active, vm.status)}</span>
                    </div>
                    <div className={styles.vmSpecWithDetails}>
                      <div className={styles.vmSpec}>
                        <FontAwesomeIcon icon={faCalendarAlt} className={styles.specIcon} />
                        <span>Created: {formatDate(vm.created_at)}</span>
                      </div>
                      <span className={styles.vmDetailsLink}>
                        Details and Connection <FontAwesomeIcon icon={faChevronRight} />
                      </span>
                    </div>
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>
      </div>
    </div>
  );
}

export default DashboardPage;