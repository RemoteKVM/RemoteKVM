import React, { useState, useEffect } from 'react';
import { Link } from 'react-router-dom';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { faUser, faLock, faSignInAlt, faUserPlus } from '@fortawesome/free-solid-svg-icons';
import './login.css';
import TopMenu from './TopMenu';

function Login() {
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [isVisible, setIsVisible] = useState(false);
  const [loginError, setLoginError] = useState('');
  
  const [prevUsername, setPrevUsername] = useState('');
  const [prevPassword, setPrevPassword] = useState('');

  // Clear login errors only when username is actually modified
  useEffect(() => {
    // Only clear error if username actually changed (not on first render)
    if (prevUsername !== '' && username !== prevUsername && loginError) {
      setLoginError('');
    }
    // Update previous username
    setPrevUsername(username);
  }, [username]);

  // Clear login errors only when password is actually modified
  useEffect(() => {
    // Only clear error if password actually changed (not on first render)
    if (prevPassword !== '' && password !== prevPassword && loginError) {
      setLoginError('');
    }
    // Update previous password
    setPrevPassword(password);
  }, [password]);

  

  useEffect(() => {
    // make visable after a small delay for animation effect
    const timer = setTimeout(() => {
      setIsVisible(true);
    }, 100);
    
    // Adjust body styles for login page
    document.body.style.overflow = 'hidden';
    document.body.classList.add('login-page-active');
    
    return () => {
      clearTimeout(timer);
      document.body.style.overflow = '';
      document.body.classList.remove('login-page-active');
    };
  }, []);

  const handleLogin = async (e) => {
    e.preventDefault();
    setLoginError('');
    
    // Custom validation
    if (!username) {
      setLoginError('Username is required');
      showErrorAnimation();
      return;
    }
    
    if (!password) {
      setLoginError('Password is required');
      showErrorAnimation();
      return;
    }
    
    try {
      const response = await fetch('/api/login', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ username, password }),
      });
      
      const data = await response.json();
      
      if (data.success) {
        // Store user information including token in localStorage
        localStorage.setItem('userId', data.userId);
        localStorage.setItem('username', username);
        localStorage.setItem('isLoggedIn', 'true');
        localStorage.setItem('token', data.token); // Store the JWT token
        
        // login success animation
        const loginForm = document.querySelector('.login-form-container');
        loginForm.classList.add('login-success');
        
        // Redirect after animation completes
        setTimeout(() => {
          window.location.href = '/dashboard';
        }, 800);
      } else {
        setLoginError(data.message || 'Invalid credentials');
        showErrorAnimation();
      }
    } catch (err) {
      setLoginError('Connection error. Please try again later.');
      showErrorAnimation();
    }
  };
  
  // Helper function for showing error animation
  const showErrorAnimation = () => {
    const loginForm = document.querySelector('.login-form-container');
    loginForm.classList.add('login-error');
    setTimeout(() => {
      loginForm.classList.remove('login-error');
    }, 500);
  };

  return (
    <div className="login-page-wrapper">
      <div className="login-top-menu-wrapper">
        <TopMenu />
      </div>
      <div className="login-page">
        <div className={`login-container ${isVisible ? 'visible' : ''}`}>
          <div className="login-form-container">
            <h2>Login</h2>
            
            {loginError && (
              <div className="login-error-message">
                {loginError}
              </div>
            )}
            
            <form onSubmit={handleLogin} noValidate>
              <div className="input-group-login-page">
                <div className="input-icon-login-page">
                  <FontAwesomeIcon icon={faUser} />
                </div>
                <input
                  type="text"
                  placeholder="Username"
                  value={username}
                  onChange={(e) => setUsername(e.target.value)}
                />
              </div>
              
              <div className="input-group-login-page">
                <div className="input-icon-login-page">
                  <FontAwesomeIcon icon={faLock} />
                </div>
                <input
                  type="password"
                  placeholder="Password"
                  value={password}
                  onChange={(e) => setPassword(e.target.value)}
                />
              </div>
              
              <button type="submit" className="login-button">
                <FontAwesomeIcon icon={faSignInAlt} /> Login
              </button>
            </form>
            
            <div className="login-footer">
              <p>Don't have an account?</p>
              <Link to="/signup" className="signup-link">
                <FontAwesomeIcon icon={faUserPlus} /> Sign Up
              </Link>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

export default Login;