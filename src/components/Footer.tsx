import React from 'react';
import type { PageRoute, User } from '../types';
import { CUTCRAFT_LOGO_SRC } from './Navbar';

interface FooterProps {
  onNavigate: (page: PageRoute) => void;
  user: User | null;
}

export const Footer: React.FC<FooterProps> = ({ onNavigate, user }) => {
  return (
    <footer className="border-t border-white/[0.08] bg-[#040610]/80 mt-20 pb-24 md:pb-10">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-10 flex flex-col md:flex-row items-start md:items-center justify-between gap-6">
        <div className="flex items-center gap-3">
          <img
            src={CUTCRAFT_LOGO_SRC}
            alt="CUTCRAFT logo"
            referrerPolicy="no-referrer"
            className="w-10 h-10 rounded-full object-cover border border-amber-400/40"
          />
          <div>
            <p className="font-display text-base font-bold text-white tracking-tight">CUTCRAFT</p>
            <p className="text-xs text-slate-400">Where Creativity Meets Opportunity.</p>
          </div>
        </div>

        <div className="flex flex-wrap items-center gap-6 text-xs text-slate-400">
          <button type="button" onClick={() => onNavigate('landing')} className="hover:text-white transition-colors">
            Home
          </button>
          <button type="button" onClick={() => onNavigate('discover')} className="hover:text-white transition-colors">
            Explore Creators
          </button>
          {user ? (
            <button type="button" onClick={() => onNavigate('projects')} className="hover:text-white transition-colors">
              Projects
            </button>
          ) : (
            <button type="button" onClick={() => onNavigate('login')} className="hover:text-white transition-colors">
              Sign In
            </button>
          )}
          <button type="button" onClick={() => onNavigate('about')} className="hover:text-amber-300 transition-colors">
            About the Developers
          </button>
        </div>

        <p className="text-xs text-slate-500">© 2026 CUTCRAFT. Built with C &amp; SQLite.</p>
      </div>
    </footer>
  );
};
