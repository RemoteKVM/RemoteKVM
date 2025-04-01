import React from 'react';
import { BrowserRouter as Router, Route, Routes } from 'react-router-dom';
import WelcomePage from './pages/WelcomePage';
import Login from './components/login';
import Signup from './components/signup';
import DashboardPage from './pages/DashboardPage';
import VMDetailPage from './pages/VMDetailPage';
import CreateVM from './pages/CreateVMPage';

function App() {
  return (
    <Router>
      <Routes>
        <Route path="/" element={<WelcomePage />} />
        <Route path="/login" element={<Login />} />
        <Route path="/signup" element={<Signup />} />
        <Route path="/dashboard" element={<DashboardPage />} />
        <Route path="/create-vm" element={<CreateVM />} />
        <Route path="/vm/:id" element={<VMDetailPage />} />
      </Routes>
    </Router>
  );
}

export default App;