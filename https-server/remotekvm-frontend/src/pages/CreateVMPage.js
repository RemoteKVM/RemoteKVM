import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { 
  faServer, faHdd, faFont, faExclamationTriangle,
  faArrowLeft, faCheck
} from '@fortawesome/free-solid-svg-icons';
import TopMenu from '../components/TopMenu';
import { faUbuntu } from '@fortawesome/free-brands-svg-icons';
import './CreateVMPage.css';

function CreateVMPage() {
  const [isVisible, setIsVisible] = useState(false);
  const [vmName, setVmName] = useState('');
  const [diskSize, setDiskSize] = useState(10);
  const [isCreating, setIsCreating] = useState(false);
  const [error, setError] = useState('');
  const [successMessage, setSuccessMessage] = useState('');
  const [isNameValid, setIsNameValid] = useState(true);
  const navigate = useNavigate();

  useEffect(() => {
    const timer = setTimeout(() => {
      setIsVisible(true);
    }, 100);
    
    return () => clearTimeout(timer);
  }, []);
  const handleVmNameChange = (e) => {
    const inputValue = e.target.value;
    // Replace spaces with underscores
    const newName = inputValue.replace(/\s+/g, '_');
    setVmName(newName);
    
    // Check if empty or valid
    if (newName === '' || isValidVmName(newName)) {
      setIsNameValid(true);
    } else {
      setIsNameValid(false);
    }
  };

  const isValidVmName = (name) => {
    // Validate VM name against regex: only letters, numbers, underscore, hyphen, and period
    const regex = /^[a-zA-Z0-9_\-.]+$/;
    return regex.test(name);
  };

  const handleCreateVM = async (e) => {
    e.preventDefault();
    setError('');
    setSuccessMessage('');
    
    // Validation
    if (!vmName.trim()) {
      setError('Please enter a VM name');
      showErrorAnimation();
      return;
    }
    
    // More detailed VM name validation
    if (vmName.length < 3 || vmName.length > 30) {
      setError('VM name must be between 3 and 30 characters');
      showErrorAnimation();
      return;
    }
    
    // Check VM name against regex pattern
    if (!isValidVmName(vmName)) {
      setError('VM name can only contain letters, numbers, underscores, hyphens, and periods');
      showErrorAnimation();
      return;
    }
    
    if (diskSize < 2 || diskSize > 25) {
      setError('Disk size must be between 2MB and 25MB');
      showErrorAnimation();
      return;
    }
    
    try {
      setIsCreating(true);
  
      // Get token from localStorage
      const token = localStorage.getItem('token');
      
      if (!token) {
        setError('You must be logged in to create a VM');
        showErrorAnimation();
        setIsCreating(false);
        return;
      }
      
      const response = await fetch('/api/create-vm', {
        method: 'POST',
        headers: { 
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${token}`
        },
        body: JSON.stringify({
          vmName,
          diskSize
        })
      });
      
      const data = await response.json();
      
      if (data.success) {
        setSuccessMessage(`VM "${vmName}" created successfully!`);
        showSuccessAnimation();
        
        // Reset form
        setVmName('');
        setDiskSize(10);
        
        // Redirect to dashboard after a delay
        setTimeout(() => {
          navigate('/dashboard');
        }, 2000);
      } else {
        setError(data.message || 'Failed to create VM');
        showErrorAnimation();
      }
    } catch (err) {
      console.error('Error creating VM:', err);
      setError('Connection error. Please try again later.');
      showErrorAnimation();
    } finally {
      setIsCreating(false);
    }
  };
  
  const handleBackToDashboard = () => {
    navigate('/dashboard');
  };

  // Helper function for showing error animation
  const showErrorAnimation = () => {
    const form = document.querySelector('.create-vm-form');
    form.classList.add('create-vm-error');
    setTimeout(() => {
      form.classList.remove('create-vm-error');
    }, 500);
  };
  
  // Helper function for showing success animation
  const showSuccessAnimation = () => {
    const form = document.querySelector('.create-vm-form');
    form.classList.add('create-vm-success');
  };

  return (
    <div className="create-vm-page-wrapper">
      <div className="create-vm-top-menu-wrapper">
        <TopMenu />
      </div>
      <div className="create-vm-page">
        <div className={`create-vm-container ${isVisible ? 'visible' : ''}`}>
          <div className="create-vm-header">
            <button 
              className="back-to-dashboard" 
              onClick={handleBackToDashboard}
              aria-label="Back to dashboard"
            >
              <FontAwesomeIcon icon={faArrowLeft} /> Back to Dashboard
            </button>
            <h2>Create Virtual Machine</h2>
          </div>
          
          <div className="create-vm-form">
            {error && (
              <div className="create-vm-error-message">
                <FontAwesomeIcon icon={faExclamationTriangle} /> {error}
              </div>
            )}
            
            {successMessage && (
              <div className="create-vm-success-message">
                <FontAwesomeIcon icon={faCheck} /> {successMessage}  
              </div>
            )}
            
            <form onSubmit={handleCreateVM}>
              <div className="form-group">
                <label htmlFor="vm-name">
                  <FontAwesomeIcon icon={faFont} /> VM Name
                </label>
                <input
                  id="vm-name"
                  type="text"
                  value={vmName}
                  onChange={handleVmNameChange}
                  placeholder="Enter VM name (e.g., WebServer)"
                  disabled={isCreating}
                  className={!isNameValid ? 'invalid-input' : ''}
                />
                <small>Choose a descriptive name between 3-30 characters. The name needs to be unic.</small>
                {!isNameValid && (
                  <div className="input-error-message">
                    <FontAwesomeIcon icon={faExclamationTriangle} /> Only letters, numbers, underscores, hyphens, and periods are allowed.
                  </div>
                )}
              </div>
              
              <div className="form-group">
                <label htmlFor="disk-size">
                  <FontAwesomeIcon icon={faHdd} /> Disk Size (MB)
                </label>
                <div className="slider-container">
                  <input
                    id="disk-size"
                    type="range"
                    min="2"
                    max="25"
                    value={diskSize}
                    onChange={(e) => setDiskSize(parseInt(e.target.value))}
                    disabled={isCreating}
                  />
                  <span className="disk-size-value">{diskSize} MB</span>
                </div>
                <small>Choose between 2MB and 25MB of storage</small>
              </div>
              
              <div className="vm-specs-preview">
                <h4>VM Configuration</h4>
                <div className="spec-item">
                  <FontAwesomeIcon icon={faUbuntu} />
                  <span>Linux Ubuntu</span>
                </div>
                <div className="spec-item">
                  <FontAwesomeIcon icon={faHdd} />
                  <span>{diskSize} MB Storage</span>
                </div>
              </div>
              
              <button 
                type="submit" 
                className="create-vm-button"
                disabled={isCreating}
              >
                {isCreating ? (
                  <>
                    <div className="button-spinner"></div>
                    Creating...
                  </>
                ) : (
                  <>
                    <FontAwesomeIcon icon={faServer} />
                    Create VM
                  </>
                )}
              </button>
            </form>
          </div>
        </div>
      </div>
    </div>
  );
}

export default CreateVMPage;