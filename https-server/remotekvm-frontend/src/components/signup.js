import React, { useState, useEffect } from 'react';
import { Link } from 'react-router-dom';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { faUser, faLock, faUserPlus, faSignInAlt, faExclamationTriangle } from '@fortawesome/free-solid-svg-icons';
import './signup.css';
import TopMenu from './TopMenu';

function Signup() {
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [confirmPassword, setConfirmPassword] = useState('');
  const [isVisible, setIsVisible] = useState(false);
  const [signupError, setSignupError] = useState('');  
  const [activeField, setActiveField] = useState(null);
  
  // Define validation patterns
  const validUsernamePattern = /^[a-zA-Z0-9_\-.]+$/;
  const validPasswordPattern = /^[a-zA-Z0-9!@#$%^&*()_+\-=[\]{};':"\\|,.<>/?]+$/;
  
  useEffect(() => {
    const timer = setTimeout(() => {
      setIsVisible(true);
    }, 100);
    
    // Adjust body styles for signup page
    document.body.style.overflow = 'hidden';
    document.body.classList.add('signup-page-active');
    
    return () => {
      clearTimeout(timer);
      document.body.style.overflow = '';
      document.body.classList.remove('signup-page-active');
    };
  }, []);
  const [prevUsername, setPrevUsername] = useState('');

    // Clear username-related errors only when username is actually modified
    useEffect(() => {
    // Only clear error if username actually changed (not on first render)
    if (prevUsername !== '' && username !== prevUsername && signupError) {
        setSignupError('');
    }
    // Update previous username
    setPrevUsername(username);
    }, [username]);

    // Track previous password/confirmPassword values
    const [prevPassword, setPrevPassword] = useState('');
    const [prevConfirmPassword, setPrevConfirmPassword] = useState('');

    // Clear password-related errors only when password is actually modified
    useEffect(() => {
    // Only clear error if password actually changed (not on first render)
    if (prevPassword !== '' && password !== prevPassword && signupError) {
        setSignupError('');
    }
    // Update previous password
    setPrevPassword(password);
    }, [password]);

    // Clear confirm password errors only when confirmPassword is actually modified
    useEffect(() => {
    // Only clear error if confirmPassword actually changed (not on first render)
    if (prevConfirmPassword !== '' && confirmPassword !== prevConfirmPassword && signupError) {
        setSignupError('');
    }
    // Update previous confirmPassword
    setPrevConfirmPassword(confirmPassword);
    }, [confirmPassword]);
  

  const handleSignup = async (e) => {
    e.preventDefault();
    setSignupError('');
    
    // Custom validation
    if (!username) {
      setSignupError('Username is required');
      showErrorAnimation();
      return;
    }
    
    if (username.length < 4) {
      setSignupError('Username must be at least 4 characters long');
      showErrorAnimation();
      return;
    }
    
    if (!validUsernamePattern.test(username)) {
      setSignupError('Username can only contain letters, numbers, and common symbols (_, -, .)');
      showErrorAnimation();
      return;
    }
    
    /*if (usernameAvailable === false) {
      setSignupError('Username is already taken');
      showErrorAnimation();
      return;
    }*/
    
    if (!password) {
      setSignupError('Password is required');
      showErrorAnimation();
      return;
    }
    
    // password length check
    if (password.length < 8) {
      setSignupError('Password must be at least 8 characters long');
      showErrorAnimation();
      return;
    }
    
    if (!validPasswordPattern.test(password)) {
      setSignupError('Password contains invalid characters');
      showErrorAnimation();
      return;
    }
    
    if (password !== confirmPassword) {
      setSignupError('Passwords do not match');
      showErrorAnimation();
      return;
    }
    
    try {
      const response = await fetch('/api/signup', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ username, password }),
      });
      
      const data = await response.json();
      
      if (data.success) {
        // Store user information in localStorage
      localStorage.setItem('userId', data.userId);
      localStorage.setItem('username', username);
      localStorage.setItem('isLoggedIn', 'true');
      localStorage.setItem('token', data.token); // Store the JWT token
      
      // signup success animation
      const signupForm = document.querySelector('.signup-form-container');
      signupForm.classList.add('signup-success');
        
        // Redirect after animation completes
        setTimeout(() => {
          window.location.href = '/dashboard';
        }, 800);
      } else {
        setSignupError(data.message || 'Signup failed');
        showErrorAnimation();
      }
    } catch (err) {
      setSignupError('Connection error. Please try again later.');
      showErrorAnimation();
    }
  };
  
  // Helper function for showing error animation
  const showErrorAnimation = () => {
    const signupForm = document.querySelector('.signup-form-container');
    signupForm.classList.add('signup-error');
    setTimeout(() => {
      signupForm.classList.remove('signup-error');
    }, 500);
  };

    // ...existing code...

    // Determine username input status for styling
    const getUsernameInputStatus = () => {
    // Show validation as soon as there's any input, not just when focused
    if (username.length === 0) return '';
    if (username.length < 4) return 'input-error';
    if (!validUsernamePattern.test(username)) return 'input-error';
    return 'input-success'; // success indicator when valid
    };

    // Determine password input status for styling
    const getPasswordInputStatus = () => {
    // Show validation as soon as there's any input
    if (password.length === 0) return '';
    if (password.length < 8) return 'input-error';
    if (!validPasswordPattern.test(password)) return 'input-error';
    return 'input-success';
    };

    // Determine confirm password input status
    const getConfirmPasswordInputStatus = () => {
    // Show validation as soon as there's any input
    if (confirmPassword.length === 0) return '';
    if (password !== confirmPassword) return 'input-error';
    return 'input-success';
    };

    // ...existing code...

  return (
    <div className="signup-page-wrapper">
      <div className="signup-top-menu-wrapper">
        <TopMenu />
      </div>
      <div className="signup-page">
        <div className={`signup-container ${isVisible ? 'visible' : ''}`}>
          <div className="signup-form-container">
            <h2>Sign Up</h2>
            
            {signupError && (
              <div className="signup-error-message">
                <FontAwesomeIcon icon={faExclamationTriangle} /> {signupError}
              </div>
            )}
            
            <form onSubmit={handleSignup} noValidate>
              <div className={`input-group ${getUsernameInputStatus()}`}>
                <div className="input-icon">
                  <FontAwesomeIcon icon={faUser} />
                </div>
                <input
                  type="text"
                  placeholder="Username"
                  value={username}
                  onChange={(e) => setUsername(e.target.value)}
                  onFocus={() => setActiveField('username')}
                  onBlur={() => setActiveField(null)}
                />
              </div>
              
              <div className={`input-group ${getPasswordInputStatus()}`}>
                <div className="input-icon">
                  <FontAwesomeIcon icon={faLock} />
                </div>
                <input
                  type="password"
                  placeholder="Password"
                  value={password}
                  onChange={(e) => setPassword(e.target.value)}
                  onFocus={() => setActiveField('password')}
                  onBlur={() => setActiveField(null)}
                />
              </div>
              
              <div className={`input-group ${getConfirmPasswordInputStatus()}`}>
                <div className="input-icon">
                  <FontAwesomeIcon icon={faLock} />
                </div>
                <input
                  type="password"
                  placeholder="Confirm Password"
                  value={confirmPassword}
                  onChange={(e) => setConfirmPassword(e.target.value)}
                  onFocus={() => setActiveField('confirmPassword')}
                  onBlur={() => setActiveField(null)}
                />
              </div>
              
              <button type="submit" className="signup-button">
                <FontAwesomeIcon icon={faUserPlus} /> Create Account
              </button>
            </form>
            
            <div className="signup-footer">
              <p>Already have an account?</p>
              <Link to="/login" className="login-link">
                <FontAwesomeIcon icon={faSignInAlt} /> Login
              </Link>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

export default Signup;