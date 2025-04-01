import React from 'react';
import { Outlet } from 'react-router-dom';
import TopMenu from './TopMenu';

function Layout({ scrollToSectionProps }) {
  return (
    <>
      <TopMenu scrollToSectionProps={scrollToSectionProps} />
      <main>
        <Outlet />
      </main>
    </>
  );
}

export default Layout;