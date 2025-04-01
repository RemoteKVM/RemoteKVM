import React, { useRef, useEffect, useState, useCallback } from 'react';
import logo from '../images/full logo only.webp';
import TopMenu from '../components/TopMenu';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { faEnvelope, faBook, faFilePowerpoint } from '@fortawesome/free-solid-svg-icons';
import { faGitlab } from '@fortawesome/free-brands-svg-icons';
import { Link, useLocation } from 'react-router-dom';
import './WelcomePage.css';


function WelcomePage() {
    // 1. First, define all your refs and state
    const terminalRef = useRef(null);
    const kvmRef = useRef(null);
    const remoteRef = useRef(null);
    const vm1Ref = useRef(null);
    const vm2Ref = useRef(null);
    const vm3Ref = useRef(null);
    const lironRef = useRef(null);
    const aboutUsRef = useRef(null);
    const omerRef = useRef(null);
    const resourcesRef = useRef(null);
    const [isTerminalVisible, setIsTerminalVisible] = useState(false);
    const [isKvmVisible, setIsKvmVisible] = useState(false);
    const [clickedSection, setClickedSection] = useState(null);
    const [activeSection, setActiveSection] = useState(null);

    const [logoLoaded, setLogoLoaded] = useState(false);

    useEffect(() => {
      const img = new Image();
      img.src = logo;
      img.onload = () => setLogoLoaded(true);
    }, []);
  

    const location = useLocation();
    
    const scrollToSection = useCallback((ref, sectionId) => {
      if (ref && ref.current) {
        // Get the height of the TopMenu
        const topMenuHeight = document.querySelector('.top-menu')?.offsetHeight || 0;
        
        // Get the element's position relative to the viewport
        const elementPosition = ref.current.getBoundingClientRect().top;
        
        // Get the current scroll position
        const offsetPosition = elementPosition + window.pageYOffset - topMenuHeight;
        
        // Smooth scroll to the adjusted position
        window.scrollTo({
          top: offsetPosition,
          behavior: 'smooth'
        });
        
        // Set the clicked section to highlight it
        setClickedSection(sectionId);
        
        const header = ref.current.querySelector('h2, h3');
        if (header) {
          setTimeout(() => {
              if (header.classList.contains('nameh3')) {
                  header.classList.add('section-header-highlight-h3');
              } else {
                  header.classList.add('section-header-highlight');
              }
              setClickedSection(null);
            }, 500); // Animation lasts 1.5 seconds
          
          // Remove the class after animation completes
          setTimeout(() => {
              if (header.classList.contains('nameh3')) {
                  header.classList.remove('section-header-highlight-h3');
              } else {
            header.classList.remove('section-header-highlight');
              }
            setClickedSection(null);
          }, 1800); // Animation lasts 1.5 seconds
        }
      }

    }, [setClickedSection]);

    useEffect(() => {
        // Check if we have state with scrollToSection
        if (location.state?.scrollToSection) {
            const sectionId = location.state.scrollToSection;
            
            // Clear the state to prevent scrolling on refresh
            window.history.replaceState({}, document.title);
            
            // Wait for DOM to be ready
            setTimeout(() => {
                switch(sectionId) {
                    case 'about-us':
                        scrollToSection(aboutUsRef, 'about-us');
                        break;
                    case 'resources':
                        scrollToSection(resourcesRef, 'resources');
                        break;
                    default:
                        break;
                }
            }, 100);
        }
    }, [location, scrollToSection]);
    
        useEffect(() => {
            const observeSections = () => {
              const sections = document.querySelectorAll('section[id]');
              
              const sectionObserver = new IntersectionObserver((entries) => {
                entries.forEach(entry => {
                  if (entry.isIntersecting) {
                    // Just update active section state, don't highlight
                    setActiveSection(entry.target.id);
                  }
                });
              }, { threshold: 0.3 });
              
              // Start observing each section
              sections.forEach(section => {
                sectionObserver.observe(section);
              });
              
              return () => {
                sections.forEach(section => {
                  sectionObserver.unobserve(section);
                });
              };
            };
            
            // Wait a moment for DOM to be ready
            setTimeout(observeSections, 100);
          }, []);

    // 2. Second, define your content arrays
    // Terminal content arrays
    const vm1Content = [
        "$ cd magshimim/remote-kvm",
        "$ ls -l /home/omer",
        "$ cat logs/system-status.log",
        "$ cd magshimim/remote-kvm",   
        "$ ls -l /home/omer  ",
        "$ cat logs/system-status.log  ",
        "$ nano config/kvm-settings.conf  ",
        "$ tail -f logs/error.log  ",
        "$ grep \"error\" logs/system.log  ",
        "$ curl -X GET https://remotekvm.online  ",
        "$ sudo systemctl restart kvm-service  ",
        "$ chmod +x scripts/start-vm.sh  ",
        "$ ./scripts/start-vm.sh testVM  ",
        "$ ping -c 4 192.168.1.100  ",
        "$ netstat -tulnp | grep ssh  ",
        "$ who | grep \"noga\"  ",
        "$ ps aux | grep kvm  ",
        "$ df -h /mnt/storage  ",
        "$ du -sh /home/liron  ",
        "$ echo \"Remote KVM is active\"  ",
        "$ touch ~/notes/security-reminder.txt  ",
        "$ mv testVM.img /mnt/storage/  ",
        "$ rm -rf old-backups/  ",
        "$ ssh jacob@192.168.1.200  ",
        "$ scp backup.tar.gz tal@server:/data  ",
        "$ sudo shutdown -r now  ",
        // ... rest of VM1 content
    ];

    const vm2Content = [
        "$ cat /var/log/syslog",
        "$ cd /var/log/kvm  ", 
        "$ ls -lh remote-connections.log  ",
        "$ cat /etc/kvm/network.conf  ",
        "$ nano ~/scripts/start-remote.sh  ",
        "$ tail -f logs/security.log  ",
        "$ grep \"failed\" logs/auth.log  ",
        "$ curl -X POST https://remotekvm.online  ",
        "$ sudo systemctl status kvm-server  ",
        "$ chmod 755 scripts/deploy.sh  ",
        "$ ./scripts/deploy.sh --force-update  ",
        "$ ping -c 3 remotekvm.online  ",
        "$ netstat -an | grep 22  ",
        "$ who | grep \"fridman\"  ",
        "$ ps -aux | grep ssh  ",
        "$ df -h /home/omer  ",
        "$ du -sh ~/Downloads/backup.tar  ",
        "$ echo \"System update in progress\"  ",
        "$ touch ~/configs/vm-settings.conf  ",
        "$ mv logs/*.log ~/backup/  ",
        "$ rm -rf /tmp/test-vm/  ",
        "$ ssh noga@192.168.1.250  ",
        "$ scp test.img tal@192.168.1.10:/vms  ",
        "$ sudo reboot now  ",
        // ... rest of VM2 content
    ];

    const vm3Content = [
        "$ cd /home/liron/vms"  , 
        "$ ls -lh test-vm.img  ", 
        "$ cat ~/.ssh/authorized_keys  ", 
        "$ nano ~/scripts/manage-kvm.sh  ", 
        "$ tail -f ~/logs/network.log  ", 
        "$ grep \"jacob\" ~/logs/access.log  ", 
        "$ curl -X DELETE https://remotekvm.online  ", 
        "$ sudo systemctl restart sshd.service  ", 
        "$ chmod +x ~/scripts/setup.sh  ", 
        "$ ./scripts/setup.sh --install-vm  ", 
        "$ ping -c 5 8.8.8.8  ", 
        "$ netstat -tulnp | grep 80  ", 
        "$ who | grep \"shvartzvald\"  ", 
        "$ ps aux | grep \"remote\"  ", 
        "$ df -Th /mnt/storage  ", 
        "$ du -sh ~/Downloads/iso/  ", 
        "$ echo \"Hypervisor maintenance mode enabled\"  ", 
        "$ touch ~/notes/magshimim-project.txt  ", 
        "$ mv ~/images/kvm.png ~/backup/  ", 
        "$ rm -rf /home/omer/temp/  ", 
        "$ ssh fridman@192.168.1.30  ", 
        "$ scp ~/backup.tar.gz noga@server:/backup  ", 
        "$ sudo shutdown -h now  ", 
    ];
    
    // 3. Define your helper functions
    const createInfiniteScrolling = (content) => {
        // Triple the content for smoother scrolling
        return [...content, ...content, ...content];
    };
    
    // 4. Create infinite content arrays
    const vm1InfiniteContent = createInfiniteScrolling(vm1Content);
    const vm2InfiniteContent = createInfiniteScrolling(vm2Content);
    const vm3InfiniteContent = createInfiniteScrolling(vm3Content);

    // 5. Finally, define your useEffect
    useEffect(() => {
        const terminalObserver = new IntersectionObserver((entries) => {
            if (entries[0].isIntersecting) {
                setIsTerminalVisible(true);
                terminalObserver.disconnect();
            }
        }, { threshold: 0.3 });
        
        const kvmObserver = new IntersectionObserver((entries) => {
            if (entries[0].isIntersecting) {
                setIsKvmVisible(true);
                kvmObserver.disconnect();
            }
        }, { threshold: 0.3 });
        
        const terminalElement = terminalRef.current;
        const kvmElement = kvmRef.current;
        
        if (terminalElement) {
            terminalObserver.observe(terminalElement);
        }
        
        if (kvmElement) {
            kvmObserver.observe(kvmElement);
        }
        
        // Create a function to handle line recycling
        const setupInfiniteScroll = (terminalRef) => {
            if (!terminalRef || !terminalRef.current || !isKvmVisible) return null;
            
            const terminal = terminalRef.current;
            
            const checkAndRecycleLines = () => {
                if (!terminal) return;
                
                const lines = terminal.querySelectorAll('.vm-terminal-line');
                if (lines.length === 0) return;
                
                const firstLine = lines[0];
                const rect = firstLine.getBoundingClientRect();
                const terminalRect = terminal.getBoundingClientRect();
                
                // If first line is above the visible area
                if (rect.bottom < terminalRect.top) {
                    // Clone and append to bottom
                    const clonedLine = firstLine.cloneNode(true);
                    terminal.appendChild(clonedLine);
                    
                    // Remove first line
                    firstLine.remove();
                }
            };
            
            // Check every 100ms
            return setInterval(checkAndRecycleLines, 100);
        };
        
        // Set up infinite scrolling after VM animations finished
        let vm1Interval, vm2Interval, vm3Interval;
        
        if (isKvmVisible) {
            // Delay to let VMs appear first
            setTimeout(() => {
                vm1Interval = setupInfiniteScroll(vm1Ref);
                vm2Interval = setupInfiniteScroll(vm2Ref);
                vm3Interval = setupInfiniteScroll(vm3Ref);
            }, 1000);
        }
        
        return () => {
            if (terminalElement) {
                terminalObserver.unobserve(terminalElement);
            }
            if (kvmElement) {
                kvmObserver.unobserve(kvmElement);
            }
            
            if (vm1Interval) clearInterval(vm1Interval);
            if (vm2Interval) clearInterval(vm2Interval);
            if (vm3Interval) clearInterval(vm3Interval);
        };
    }, [isKvmVisible]); // Only depend on isKvmVisible, not the content arrays


    useEffect(() => {
        const observeSections = () => {
          const sections = document.querySelectorAll('section[id]');
          
          const sectionObserver = new IntersectionObserver((entries) => {
            entries.forEach(entry => {
              if (entry.isIntersecting) {
                // Just update active section state, don't highlight
                setActiveSection(entry.target.id);
              }
            });
          }, { threshold: 0.3 });
          
          // Start observing each section
          sections.forEach(section => {
            sectionObserver.observe(section);
          });
          
          return () => {
            sections.forEach(section => {
              sectionObserver.unobserve(section);
            });
          };
        };
        
        // Wait a moment for DOM to be ready
        setTimeout(observeSections, 100);
      }, []);
      
  return (
    <>
      <TopMenu 
                scrollToSectionProps={{
                    scrollToSection: scrollToSection,
                    aboutUsRef: aboutUsRef,
                    resourcesRef: resourcesRef
                }} 
            />
      
        {/* Hero Section */}
    <section className="section hero-section" style={{ textAlign: 'center', padding: 0 }}>
        {/* Remove the .container / .row / .col wrappers */}
        <div className="welcome-content-box">
            <img src={logo} alt="RemoteKVM Logo" className="welcome-logo" />
            <div className="title-and-subtitle">
            <h1 className="welcome-title">
                <span 
                  className={`light-green-text interactive-title ${activeSection === 'remote' ? 'active-section' : ''}`}

                  onClick={() => scrollToSection(remoteRef)}
                >
                  Remote
                </span>
                <span 
                  className={`dark-green-text interactive-title ${activeSection === 'kvm' ? 'active-section' : ''}`}
                  onClick={() => scrollToSection(kvmRef)}
                >
                  KVM
                </span>
            </h1>
            <div className="created-by-container">
                <p className="created-by">
                <span className='dark-green-text interactive-title'
                    onClick={() => scrollToSection(lironRef, 'liron')}>Liron Fridman</span>
                <span className='blue-text interactive-title' onClick={() => scrollToSection(aboutUsRef, 'about-us')}> & </span>
                <span className='light-green-text interactive-title' onClick={() => scrollToSection(omerRef, 'omer')}>Omer Shvartzvald</span>
                </p>
            </div>
            </div>
        </div>

          {/* About text within hero section */}
          <div className="row justify-content-center">
            <div className="col-12 col-md-5 ">
              <div className="about-text center-text">
                <p><strong className="light-green-text bold">Remote</strong>
                <strong className="dark-green-text">KVM</strong> is a 12th grade project at magshimim Cyber that was created by Liron Fridman and Omer Shvartzvald.</p>
                <p>The project is a type 2 hypervisor (a software that creates and runs virtual machines) implemented using KVM.</p>
                <p>You are invited to <a href='/signup' className='light-green-text'>create a user</a> on our website, create a virtual machine, and connect to it using SSH!</p>
                <p>Do to the nature of the project, where the hypervisor is running on Liron's/Omer's laptop, This website will not always demonstrate the full functionality of the project. If relevent you are welcomed to <span 
  className='light-green-text'
  onClick={() => scrollToSection(aboutUsRef, 'about-us')}
  style={{ cursor: 'pointer', textDecoration: 'underline' }}
>contact us :)</span></p>
              </div>
            </div>
          </div>

      </section>

      <section className="section remote-section" id="remote" ref={remoteRef} style={{ textAlign: 'center', padding: 0 }}>
  <h2 className='font-optical-sizing-off' >Remote</h2>
  <div className="terminal-container" ref={terminalRef}>
          <div className={`terminal-window ${isTerminalVisible ? 'terminal-visible' : ''}`}>
            <div className="terminal-header">
              <div className="terminal-button terminal-close"></div>
              <div className="terminal-button terminal-minimize"></div>
              <div className="terminal-button terminal-maximize"></div>
              <div className="terminal-title">RemoteKVM</div>
            </div>
            <div className="terminal-content">
              <div className="terminal-line">$ ssh user@remotekvm.online</div>
              <div className={`terminal-line ${isTerminalVisible ? 'terminal-typing' : ''}`}>Connecting to remote host...</div>
              <div className={`terminal-line ${isTerminalVisible ? 'terminal-response' : ''}`}>Authentication successful</div>
              <div className={`terminal-line ${isTerminalVisible ? 'terminal-response' : ''}`}>Welcome to RemoteKVM!</div>
              <div className={`terminal-line ${isTerminalVisible ? 'terminal-response' : ''}`}>You are connected to our C++ self implemented SSH Server</div>
              <div className={`terminal-line ${isTerminalVisible ? 'terminal-cursor' : ''}`}>$</div>
            </div>
          </div>
        </div>
  
  {/* About text within hero section */}
  <div className="row justify-content-center">
    <div className="col-12 col-md-5 ">
      <div className="about-text">
        <p>Our project allows users to remotely connect to a virtual machine running on our laptop.</p>
        <p>The communication is done using the SSH protocol that we implemented ourselves in C++.</p>
        <p>After you have created a virtual machine in our dashboard, you can either connect to it directly using Putty or OpenSSH (ssh command in linux) or choose to connect to it using a virtual terminal directly from the browser.</p>
      </div>
    </div>
  </div>
</section>

       <section className="section kvm-section" id="kvm" ref={kvmRef} style={{ textAlign: 'center', padding: 0 }}>
      <h2 className='font-optical-sizing-off'>KVM</h2>
      {/* KVM Visualization */}
      <div className="hypervisor-container" ref={kvmRef}>
        <div className={`hypervisor-visualization ${isKvmVisible ? 'hypervisor-visible' : ''}`}>
          <div className="laptop">
            <div className="laptop-camera"></div>
            <div className="laptop-screen">
              <div className="laptop-screen-content">
                <div className="hypervisor-label">Type 2 Hypervisor (KVM)</div>
                <div className="vm-container">
                <div className="vm vm1">
                    <div className="vm-header">
                        <div className="vm-label">VM 1 - Noga</div>
                        <div className="vm-status-light"></div>
                    </div>
                    <div className="vm-terminal">
                        <div 
                        ref={vm1Ref}
                        className={`vm-continuous-text ${isKvmVisible ? 'vm-scrolling1' : ''}`}
                        >
                        {vm1InfiniteContent.map((line, index) => (
                            <div key={`vm1-line-${index}`} className="vm-terminal-line">{line}</div>
                        ))}
                        </div>
                    </div>
                    </div>

                    <div className="vm vm2">
                    <div className="vm-header">
                        <div className="vm-label">VM 2 - Tal</div>
                        <div className="vm-status-light"></div>
                    </div>
                    <div className="vm-terminal">
                        <div 
                        ref={vm2Ref}
                        className={`vm-continuous-text ${isKvmVisible ? 'vm-scrolling2' : ''}`}
                        >
                        {vm2InfiniteContent.map((line, index) => (
                            <div key={`vm2-line-${index}`} className="vm-terminal-line">{line}</div>
                        ))}
                        </div>
                    </div>
                    </div>

                    <div className="vm vm3">
                    <div className="vm-header">
                        <div className="vm-label">VM 3 - Yakov</div>
                        <div className="vm-status-light"></div>
                    </div>
                    <div className="vm-terminal">
                        <div 
                        ref={vm3Ref}
                        className={`vm-continuous-text ${isKvmVisible ? 'vm-scrolling3' : ''}`}
                        >
                        {vm3InfiniteContent.map((line, index) => (
                            <div key={`vm3-line-${index}`} className="vm-terminal-line">{line}</div>
                        ))}
                        </div>
                    </div>
                    </div>
                </div>
              </div>
            </div>
            <div className="laptop-base">
              <div className="laptop-keyboard"></div>
              <div className="laptop-touchpad"></div>
            </div>
          </div>
        </div>
      </div>
      
      {/* About text */}
      <div className="row justify-content-center">
        <div className="col-12 col-md-5">
          <div className="about-text">
            <p>Our hypervisor is a type 2 hypervisor which means that the virtual machines are vrunning on an OS and not on bare metal. We implemented it using KVM (kernel virtual machine).</p>
            <p>KVM allows us to create the virtual processor for the virtual machines running on our hypervisor.</p>
            <p>We had to implement several different components that would connect to our virtual machine. Serial console - so we could communicate with the virtual machine, PCI + VirtIO + VirtIO block - so we could enable disk connection and file storage on the machine.</p>
          </div>
        </div>
      </div>
    </section>

    <section className="section about-us-section" id="about-us" ref={aboutUsRef} style={{ textAlign: 'center', padding: 0 }}>
    <h2 className='font-optical-sizing-off'>About Us</h2>
          <div className="row justify-content-center">
            <div className="col-12 col-md-5 ">
              <div className="about-text center-text">
                <p>We are Liron Fridman and Omer Shvartzvald, two friends studying together at the Tel Hai center of Magshimim Cyber.</p>
              </div>
            </div>
          </div>

      </section>    
    <section className="section team-section">
      <div className="team-container">
        <div className="team-member liron-section" ref={lironRef} id="liron">
          <h3 className='nameh3'>Liron Fridman</h3>
          <div className="team-content">
    
            <div className="member-details about-text">
              <p>Liron lives on Kibbutz Ayelet Hashachar. He is a 12th grade student at Emek Hahula School and is studying physics and chemistry.</p>
              <p>During this project, Liron focused on developing the hypervisor's core functionalitys, including implementing a PCI and VirtIO virtualization.</p>
              <div className="member-contact">
                <a href="mailto:remotekvm.lo+liron@gmail.com" className="contact-link">
                <FontAwesomeIcon icon={faEnvelope} /> remotekvm.lo+liron@gmail.com
                </a>
              </div>
            </div>
          </div>
        </div>
        
        <div className="team-member omer-section" ref={omerRef} id="omer">
          <h3 className='nameh3'>Omer Shvartzvald</h3>
          <div className="team-content">
            <div className="member-details about-text">
              <p>Omer lives on Kibbutz Maayan Baruch. He is a 12th grade student at Emek Hahula School and is studying physics and computer science.</p>
              <p>during this project, Omer focused on developing the "Remote" part of the project, including the self implemented SSH server.</p>
              <div className="member-contact">
                <a href="mailto:remotekvm.lo+omer@gmail.com" className="contact-link">
                <FontAwesomeIcon icon={faEnvelope} /> remotekvm.lo+omer@gmail.com
                </a>
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>

      
        <section className="section resources-section" id="resources" ref={resourcesRef} style={{ textAlign: 'center', padding: 0 }}>
    <h2 className='font-optical-sizing-off'>Project Resources</h2>

            <div>
                <Link to="https://gitlab.com/virtualboss/tel-hai-103-virtualboss" className="menu-button dark-green-button resources-buttons"><FontAwesomeIcon icon={faGitlab} /> GitLab Repository</Link>
                <Link to="https://docs.google.com/document/d/1aULOYGXbrKZjQeXdZRdgJqwQUQY-bvm8ffIVtNzeAHA/edit?usp=sharing" className="menu-button light-green-button resources-buttons"><FontAwesomeIcon icon={faBook} /> Project Documentation</Link>
                <Link to="https://docs.google.com/presentation/d/1yuHf-qhQIX3mmBx9EBr3icslYw3DcNa-/edit?usp=sharing&ouid=106854554423523955510&rtpof=true&sd=true" className="menu-button dark-green-button resources-buttons"><FontAwesomeIcon icon={faFilePowerpoint} /> PowerPoint Presentation</Link>
            </div>
          {/* About text within hero section */}
          <div className="row justify-content-center">
            <div className="col-12 col-md-5 ">
              <div className="about-text center-text">
                <p>Check out our project source code on GitLab or browse the comprehensive documentation
                to learn more about the technical details of <strong className="light-green-text bold">Remote</strong>
                <strong className="dark-green-text">KVM</strong>.</p>
              </div>
            </div>
          </div>

      </section>

      <section className="section credits-section" id="credits" style={{ textAlign: 'center', padding: 0 }}>
    <h2 className='font-optical-sizing-off'>Credits</h2>
          <div className="row justify-content-center">
            <div className="col-12 col-md-5 ">
              <div className="about-text center-text">
                <p>We would like to thank several people who helped us in the process of creating this project:</p>
                <p><strong>Noga Anaby</strong> - Instructor and Teacher at Magshimim </p>
                <p><strong>The mentors Yakov and Tal</strong></p>
              </div>
            </div>
          </div>

      </section>
    </>
  );
}

export default WelcomePage;