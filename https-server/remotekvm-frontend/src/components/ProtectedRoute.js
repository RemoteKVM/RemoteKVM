import React, { useEffect, useState } from 'react';
import { Navigate } from 'react-router-dom';

function ProtectedRoute({ children }) {
  const [isAuthenticated, setIsAuthenticated] = useState(null);
  
  useEffect(() => {
    const checkAuth = async () => {
      const token = localStorage.getItem('token');
      
      if (!token) {
        setIsAuthenticated(false);
        return;
      }

      try {
        const response = await fetch('/api/validate-token', {
          headers: {
            'Authorization': `Bearer ${token}`
          }
        });

        const data = await response.json();
        setIsAuthenticated(data.valid);
        
        if (!data.valid) {
          // Clear invalid tokens
          localStorage.removeItem('token');
          localStorage.removeItem('userId');
          localStorage.removeItem('username');
          localStorage.removeItem('isLoggedIn');
        }
      } catch (err) {
        console.error('Auth check error:', err);
        setIsAuthenticated(false);
        // Clear localStorage on error
        localStorage.removeItem('token');
        localStorage.removeItem('userId');
        localStorage.removeItem('username');
        localStorage.removeItem('isLoggedIn');
      }
    };

    checkAuth();
  }, []);

  // Show loading state while checking authentication
  if (isAuthenticated === null) {
    return <div className="auth-loading">Authenticating...</div>;
  }

  // Redirect to login if not authenticated
  if (!isAuthenticated) {
    return <Navigate to="/login" replace />;
  }

  // Render the children if authenticated
  return children;
}

export default ProtectedRoute;