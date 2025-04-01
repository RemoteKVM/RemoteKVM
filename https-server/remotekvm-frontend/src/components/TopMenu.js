import React, { useState, useEffect, useMemo } from 'react';
import { Link, useLocation, useNavigate } from 'react-router-dom';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { 
  faSignOutAlt, 
  faTachometerAlt, 
  faBars, 
  faTimes 
} from '@fortawesome/free-solid-svg-icons';
import logoImage from '../images/remotekvm-one-line-logo.webp'; // Adjust path as needed
import './TopMenu.css';

function TopMenu({ scrollToSectionProps }) {
  const navigate = useNavigate();
  const location = useLocation();
  const [isLoggedIn, setIsLoggedIn] = useState(false);
  const [username, setUsername] = useState('');
  const [mobileMenuOpen, setMobileMenuOpen] = useState(false);

  const imageSrc = useMemo(() => logoImage, []);
  
  // Check login status on component mount
  useEffect(() => {
    const loggedIn = localStorage.getItem('isLoggedIn') === 'true';
    const storedUsername = localStorage.getItem('username');
    
    setIsLoggedIn(loggedIn);
    if (storedUsername) {
      setUsername(storedUsername);
    }
  }, []);

  // Toggle mobile menu
  const toggleMobileMenu = () => {
    setMobileMenuOpen(!mobileMenuOpen);
  };

  // Close mobile menu
  const closeMobileMenu = () => {
    setMobileMenuOpen(false);
  };

  // Handle logout
  const handleLogout = () => {
     // Clear localStorage
    localStorage.removeItem('userId');
    localStorage.removeItem('username');
    localStorage.removeItem('isLoggedIn');
    localStorage.removeItem('token');
    
    // Update component state
    setIsLoggedIn(false);
    setUsername('');
    
    // Close mobile menu
    closeMobileMenu();
    
    // Redirect to home page
    if (location.pathname !== '/') {
      navigate('/');
    } else {
      // If already on home page, force UI refresh
      navigate(0); // This will refresh the current page
    }
  };
  
  // Handle navigation and scrolling
  const handleSectionClick = (sectionId) => {
    // Close mobile menu first
    closeMobileMenu();
    
    // If not on home page, navigate there first then scroll
    if (location.pathname !== '/') {
      navigate('/', { state: { scrollToSection: sectionId } });
    } else if (scrollToSectionProps) {
      // If already on home page and scrollToSectionProps is available, scroll directly
      switch(sectionId) {
        case 'about-us':
          scrollToSectionProps.scrollToSection(scrollToSectionProps.aboutUsRef, sectionId);
          break;
        case 'resources':
          scrollToSectionProps.scrollToSection(scrollToSectionProps.resourcesRef, sectionId);
          break;
        default:
          break;
      }
    }
  };

  return (
    <div className="top-menu">
      <div className="menu-center">
        <Link to="/" onClick={closeMobileMenu}>
          <img src={imageSrc} alt="RemoteKVM Logo" className="menu-logo" />
        </Link>
      </div>
      
      {/* Hamburger menu button for mobile */}
      <div className="mobile-menu-toggle" onClick={toggleMobileMenu}>
        <FontAwesomeIcon icon={mobileMenuOpen ? faTimes : faBars} />
      </div>
      
      {/* Desktop menu and mobile dropdown */}
      <div className={`menu-container ${mobileMenuOpen ? 'open' : ''}`}>
        <div className="mobile-menu-content">
          <div className="menu-left">
            <span 
              onClick={() => handleSectionClick('about-us')} 
              className="menu-link"
              style={{ cursor: 'pointer' }}
            >
              About Us
            </span>
            <span 
              onClick={() => handleSectionClick('resources')} 
              className="menu-link"
              style={{ cursor: 'pointer' }}
            >
              Project Resources
            </span>
          </div>
          
          <div className="menu-right">
            {isLoggedIn ? (
              <>
                <Link to="/dashboard" className="menu-button dark-green-button" onClick={closeMobileMenu}>
                  <FontAwesomeIcon icon={faTachometerAlt} /> Dashboard
                </Link>
                <button onClick={handleLogout} className="menu-button light-green-button">
                  <FontAwesomeIcon icon={faSignOutAlt} /> Logout
                </button>
              </>
            ) : (
              <>
                <Link to="/login" className="menu-button dark-green-button" onClick={closeMobileMenu}>Login</Link>
                <Link to="/signup" className="menu-button light-green-button" onClick={closeMobileMenu}>Sign Up</Link>
              </>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}

export default TopMenu;