import { useEffect, useState } from 'react';
import { api, getStoredToken, setStoredToken } from './api/client';
import { Footer } from './components/Footer';
import { MediaModal } from './components/MediaModal';
import { Navbar } from './components/Navbar';
import { AboutDevelopersPage } from './pages/AboutDevelopersPage';
import { AuthPages } from './pages/AuthPages';
import { ChatPage } from './pages/ChatPage';
import { DashboardPage } from './pages/DashboardPage';
import { DiscoveryPage } from './pages/DiscoveryPage';
import { FreelancerProfilePage } from './pages/FreelancerProfilePage';
import { LandingPage } from './pages/LandingPage';
import { ProjectRequestPage } from './pages/ProjectRequestPage';
import { ProjectsPage } from './pages/ProjectsPage';
import { SettingsPage } from './pages/SettingsPage';
import type { Freelancer, PageRoute, PortfolioItem, User } from './types';

export default function App() {
  const [currentPage, setCurrentPage] = useState<PageRoute>('landing');
  const [user, setUser] = useState<User | null>(null);
  const [authReady, setAuthReady] = useState(false);

  const [featured, setFeatured] = useState<Freelancer[]>([]);
  const [trending, setTrending] = useState<PortfolioItem[]>([]);
  const [landingLoading, setLandingLoading] = useState(true);

  const [discoveryCategory, setDiscoveryCategory] = useState('All');
  const [selectedFreelancerId, setSelectedFreelancerId] = useState<number>(1);
  const [hireTarget, setHireTarget] = useState<Freelancer | null>(null);
  const [chatPeerUserId, setChatPeerUserId] = useState<number | null>(null);
  const [modalPortfolio, setModalPortfolio] = useState<PortfolioItem | null>(null);

  useEffect(() => {
    const token = getStoredToken();
    if (!token) {
      setAuthReady(true);
    } else {
      api
        .me()
        .then((res) => setUser(res.user))
        .catch(() => setStoredToken(null))
        .finally(() => setAuthReady(true));
    }

    Promise.all([api.getFeaturedFreelancers(), api.getTrendingPortfolios()])
      .then(([fRes, pRes]) => {
        setFeatured(fRes.freelancers);
        setTrending(pRes.portfolios);
      })
      .catch(() => {})
      .finally(() => setLandingLoading(false));
  }, []);

  const navigate = (page: PageRoute) => {
    setCurrentPage(page);
    window.scrollTo({ top: 0, behavior: 'smooth' });
  };

  const handleSelectCategory = (category: string) => {
    setDiscoveryCategory(category);
    navigate('discover');
  };

  const handleSelectFreelancer = (id: number) => {
    setSelectedFreelancerId(id);
    navigate('profile');
  };

  const handleHireNow = (freelancer: Freelancer) => {
    setHireTarget(freelancer);
    navigate('request');
  };

  const handleMessageUser = (peerUserId: number) => {
    if (!user) {
      navigate('login');
      return;
    }
    setChatPeerUserId(peerUserId);
    navigate('chat');
  };

  const handleLogout = async () => {
    try {
      await api.logout();
    } catch {
      /* ignore */
    }
    setStoredToken(null);
    setUser(null);
    navigate('landing');
  };

  return (
    <div className="min-h-screen liquid-canvas flex flex-col justify-between">
      <Navbar currentPage={currentPage} user={user} onNavigate={navigate} />

      <main className="flex-1">
        {!authReady ? (
          <div className="max-w-7xl mx-auto px-4 py-16">
            <div className="glass-panel rounded-3xl h-64 animate-pulse" />
          </div>
        ) : (
          <>
            {currentPage === 'landing' && (
              <LandingPage
                user={user}
                featured={featured}
                portfolios={trending}
                loading={landingLoading}
                onNavigate={navigate}
                onSelectCategory={handleSelectCategory}
                onSelectFreelancer={handleSelectFreelancer}
                onOpenPortfolio={setModalPortfolio}
              />
            )}

            {(currentPage === 'register' || currentPage === 'login') && (
              <AuthPages
                mode={currentPage}
                onNavigate={navigate}
                onAuthenticated={(u) => setUser(u)}
              />
            )}

            {currentPage === 'discover' && (
              <DiscoveryPage
                initialCategory={discoveryCategory}
                onSelectFreelancer={handleSelectFreelancer}
              />
            )}

            {currentPage === 'profile' && (
              <FreelancerProfilePage
                freelancerId={selectedFreelancerId}
                currentUser={user}
                onBack={() => navigate('discover')}
                onHireNow={handleHireNow}
                onMessageCreator={handleMessageUser}
                onOpenPortfolio={setModalPortfolio}
              />
            )}

            {currentPage === 'dashboard' &&
              (user ? (
                <DashboardPage
                  user={user}
                  onUserUpdated={setUser}
                  onNavigate={navigate}
                  onSelectCategory={handleSelectCategory}
                  onSelectFreelancer={handleSelectFreelancer}
                  onOpenPortfolio={setModalPortfolio}
                />
              ) : (
                <AuthPages mode="login" onNavigate={navigate} onAuthenticated={setUser} />
              ))}

            {currentPage === 'request' && (
              <ProjectRequestPage
                user={user}
                selectedFreelancer={hireTarget}
                onNavigate={navigate}
              />
            )}

            {currentPage === 'chat' &&
              (user ? (
                <ChatPage user={user} initialPeerUserId={chatPeerUserId} />
              ) : (
                <AuthPages mode="login" onNavigate={navigate} onAuthenticated={setUser} />
              ))}

            {currentPage === 'projects' &&
              (user ? (
                <ProjectsPage user={user} onMessageUser={handleMessageUser} />
              ) : (
                <AuthPages mode="login" onNavigate={navigate} onAuthenticated={setUser} />
              ))}

            {currentPage === 'settings' &&
              (user ? (
                <SettingsPage user={user} onUserUpdated={setUser} onLogout={handleLogout} />
              ) : (
                <AuthPages mode="login" onNavigate={navigate} onAuthenticated={setUser} />
              ))}

            {currentPage === 'about' && <AboutDevelopersPage />}
          </>
        )}
      </main>

      <Footer onNavigate={navigate} user={user} />

      <MediaModal
        item={modalPortfolio}
        onClose={() => setModalPortfolio(null)}
        onOpenCreator={handleSelectFreelancer}
      />
    </div>
  );
}
