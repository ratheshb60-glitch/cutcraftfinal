import React from 'react';
import { Compass, FolderKanban, Home, Info, LayoutDashboard, MessageSquare, Settings } from 'lucide-react';
import type { PageRoute, User } from '../types';

export const CUTCRAFT_LOGO_SRC = '/assets/aistudio/WhatsApp_Image_2026-11-14_at_10.23.47_PM__1__094b0fb4.jpeg';

interface NavbarProps {
  currentPage: PageRoute;
  user: User | null;
  onNavigate: (page: PageRoute) => void;
}

export const Navbar: React.FC<NavbarProps> = ({ currentPage, user, onNavigate }) => {
  const linkClass = (page: PageRoute) =>
    `text-sm font-medium whitespace-nowrap shrink-0 pb-0.5 border-b transition-colors ${
      currentPage === page
        ? 'text-amber-400 border-amber-400'
        : 'text-slate-300 border-transparent hover:text-white hover:border-white/30'
    }`;

  return (
    <>
      <header className="sticky top-0 z-40 border-b border-white/[0.08] bg-[#050814]/80 backdrop-blur-xl">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 h-14 md:h-16 flex items-center justify-between gap-8">
          {/* Zone 1: Brand Wordmark */}
          <button
            type="button"
            onClick={() => onNavigate('landing')}
            className="flex items-center gap-2.5 whitespace-nowrap shrink-0 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-amber-400 rounded-lg py-1"
          >
            <img
              src={CUTCRAFT_LOGO_SRC}
              alt="CUTCRAFT logo"
              referrerPolicy="no-referrer"
              className="w-8 h-8 rounded-full object-cover border border-amber-400/50"
            />
            <span className="font-display text-lg font-bold tracking-tight text-white">CUTCRAFT</span>
          </button>

          {/* Zone 2: 4–5 Clean Single-Line Nav Links */}
          <nav className="hidden md:flex items-center gap-7" aria-label="Primary navigation">
            <button type="button" onClick={() => onNavigate('discover')} className={linkClass('discover')}>
              Explore
            </button>
            {user ? (
              <>
                <button type="button" onClick={() => onNavigate('dashboard')} className={linkClass('dashboard')}>
                  Dashboard
                </button>
                <button type="button" onClick={() => onNavigate('projects')} className={linkClass('projects')}>
                  Projects
                </button>
                <button type="button" onClick={() => onNavigate('chat')} className={linkClass('chat')}>
                  Messages
                </button>
              </>
            ) : (
              <button type="button" onClick={() => onNavigate('login')} className={linkClass('login')}>
                Sign In
              </button>
            )}
            <button type="button" onClick={() => onNavigate('about')} className={linkClass('about')}>
              About
            </button>
          </nav>

          {/* Zone 3: 1 Primary Action */}
          <div className="flex items-center shrink-0">
            {user ? (
              <button
                type="button"
                onClick={() => onNavigate('settings')}
                className="px-4 py-2 text-xs font-semibold rounded-lg border border-white/15 bg-white/[0.06] text-slate-100 hover:border-amber-400/50 hover:text-amber-300 transition-colors whitespace-nowrap shrink-0 min-h-[40px]"
              >
                {user.full_name.split(' ')[0]} · Settings
              </button>
            ) : (
              <button
                type="button"
                onClick={() => onNavigate('register')}
                className="gold-cta px-4 py-2 text-xs rounded-lg whitespace-nowrap shrink-0 min-h-[40px]"
              >
                Get Started
              </button>
            )}
          </div>
        </div>
      </header>

      {/* Mobile Bottom Navigation Bar */}
      <nav
        aria-label="Mobile navigation"
        className="md:hidden fixed bottom-0 left-0 right-0 z-40 border-t border-white/[0.10] bg-[#060916]/92 backdrop-blur-xl px-1"
      >
        <div className="grid grid-cols-5 items-center h-16">
          <button
            type="button"
            onClick={() => onNavigate('landing')}
            className={`min-h-[44px] flex flex-col items-center justify-center gap-1 ${
              currentPage === 'landing' ? 'text-amber-400' : 'text-slate-400'
            }`}
          >
            <Home className="w-5 h-5" />
            <span className="text-[10px] font-medium">Home</span>
          </button>

          <button
            type="button"
            onClick={() => onNavigate('discover')}
            className={`min-h-[44px] flex flex-col items-center justify-center gap-1 ${
              currentPage === 'discover' ? 'text-amber-400' : 'text-slate-400'
            }`}
          >
            <Compass className="w-5 h-5" />
            <span className="text-[10px] font-medium">Explore</span>
          </button>

          <button
            type="button"
            onClick={() => onNavigate(user ? 'dashboard' : 'login')}
            className={`min-h-[44px] flex flex-col items-center justify-center gap-1 ${
              currentPage === 'dashboard' || currentPage === 'login' ? 'text-amber-400' : 'text-slate-400'
            }`}
          >
            <LayoutDashboard className="w-5 h-5" />
            <span className="text-[10px] font-medium">{user ? 'Studio' : 'Sign In'}</span>
          </button>

          <button
            type="button"
            onClick={() => onNavigate(user ? 'projects' : 'register')}
            className={`min-h-[44px] flex flex-col items-center justify-center gap-1 ${
              currentPage === 'projects' || currentPage === 'register' ? 'text-amber-400' : 'text-slate-400'
            }`}
          >
            <FolderKanban className="w-5 h-5" />
            <span className="text-[10px] font-medium">{user ? 'Projects' : 'Join'}</span>
          </button>

          <button
            type="button"
            onClick={() => onNavigate(user ? 'chat' : 'about')}
            className={`min-h-[44px] flex flex-col items-center justify-center gap-1 ${
              currentPage === 'chat' || currentPage === 'about' ? 'text-amber-400' : 'text-slate-400'
            }`}
          >
            {user ? <MessageSquare className="w-5 h-5" /> : <Info className="w-5 h-5" />}
            <span className="text-[10px] font-medium">{user ? 'Chat' : 'About'}</span>
          </button>
        </div>
      </nav>
    </>
  );
};
