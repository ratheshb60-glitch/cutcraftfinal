import React from 'react';
import { ArrowRight, Play } from 'lucide-react';
import type { Freelancer, PageRoute, PortfolioItem, User } from '../types';
import { CUTCRAFT_LOGO_SRC } from '../components/Navbar';
import { FreelancerCard } from '../components/FreelancerCard';

interface LandingPageProps {
  user: User | null;
  featured: Freelancer[];
  portfolios: PortfolioItem[];
  loading: boolean;
  onNavigate: (page: PageRoute) => void;
  onSelectCategory: (category: string) => void;
  onSelectFreelancer: (id: number) => void;
  onOpenPortfolio: (item: PortfolioItem) => void;
}

const CATEGORIES = [
  { name: 'Video Editors', desc: 'Narrative cuts, commercial reels & DaVinci Resolve grading' },
  { name: 'Photo Editors', desc: 'High-end retouching, color toning & RAW finishing' },
  { name: 'Photographers', desc: 'Editorial fashion, product & architectural shoots' },
  { name: 'Videographers', desc: 'Cinema camera direction, music videos & FPV drones' },
  { name: 'Graphic Designers', desc: 'Visual identity systems, posters & brand collateral' },
  { name: 'Motion Graphics Designers', desc: '3D broadcast titles, Cinema 4D & kinetic type' },
  { name: 'Thumbnail Designers', desc: 'High-CTR YouTube packaging & visual storytelling' },
  { name: 'Other Creatives', desc: 'Sound designers, colorists & creative directors' },
];

export const LandingPage: React.FC<LandingPageProps> = ({
  user,
  featured,
  portfolios,
  loading,
  onNavigate,
  onSelectCategory,
  onSelectFreelancer,
  onOpenPortfolio,
}) => {
  return (
    <div className="space-y-20">
      {/* Hero Section */}
      <section className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 pt-10 md:pt-16">
        <div className="grid grid-cols-1 lg:grid-cols-12 gap-8 items-stretch">
          <div className="lg:col-span-7 flex flex-col justify-center space-y-6">
            <p className="text-xs font-medium tracking-widest uppercase text-amber-400">
              CUTCRAFT Creative Marketplace
            </p>
            <h1 className="font-display text-4xl sm:text-5xl lg:text-6xl font-extrabold tracking-tight text-white leading-[1.06] text-balance">
              CUTCRAFT — Where Creativity Meets Opportunity.
            </h1>
            <p className="text-base sm:text-lg text-slate-300 max-w-2xl leading-relaxed">
              Discover, collaborate with, and commission vetted video editors, photographers, graphic artists, and 3D motion designers. Transparent project pricing, direct studio chat, and original-quality portfolios.
            </p>

            <div className="flex flex-wrap items-center gap-4 pt-2">
              <button
                type="button"
                onClick={() => onNavigate(user ? 'dashboard' : 'register')}
                className="gold-cta px-6 py-3.5 text-sm rounded-xl inline-flex items-center gap-2 min-h-[44px]"
              >
                <span>{user ? 'Open Studio Dashboard' : 'Get Started'}</span>
                <ArrowRight className="w-4 h-4" />
              </button>
              <button
                type="button"
                onClick={() => onNavigate('discover')}
                className="px-6 py-3.5 text-sm font-medium rounded-xl border border-white/15 bg-white/[0.04] text-slate-100 hover:border-amber-400/50 hover:bg-white/[0.08] transition-colors min-h-[44px]"
              >
                Explore Creators
              </button>
              {!user && (
                <button
                  type="button"
                  onClick={() => onNavigate('login')}
                  className="px-4 py-3 text-sm font-medium text-slate-300 hover:text-amber-300 transition-colors min-h-[44px]"
                >
                  Login to Account
                </button>
              )}
            </div>
          </div>

          {/* Right Column: Prominent CUTCRAFT Emblem & Visual Showcase */}
          <div className="lg:col-span-5">
            <div className="glass-panel rounded-3xl p-6 sm:p-8 h-full flex flex-col items-center justify-between text-center relative overflow-hidden">
              <div className="w-full flex items-center justify-between text-xs text-slate-400">
                <span>Frames · Cuts · Stories</span>
                <span className="font-mono-num text-amber-300">10% Escrow Commission</span>
              </div>

              <div className="my-6 relative flex items-center justify-center">
                <div className="absolute -inset-4 rounded-full bg-amber-500/10 blur-2xl pointer-events-none" />
                <img
                  src={CUTCRAFT_LOGO_SRC}
                  alt="CUTCRAFT official emblem"
                  referrerPolicy="no-referrer"
                  className="relative w-52 h-52 sm:w-60 sm:h-60 rounded-full object-cover border-2 border-amber-400/60 shadow-2xl"
                />
              </div>

              <div className="w-full pt-4 border-t border-white/[0.08] flex items-center justify-between text-xs text-slate-300">
                <span>Direct Client-Creator Hiring</span>
                <span>Original 4K File Preservation</span>
              </div>
            </div>
          </div>
        </div>

        {/* Full-Width Key Marketplace Capabilities Strip */}
        <div className="mt-10 glass-panel rounded-2xl p-5 grid grid-cols-2 lg:grid-cols-4 gap-6">
          <div>
            <p className="font-mono-num text-2xl font-bold text-white">8+</p>
            <p className="text-xs text-slate-400 mt-1">Creative Disciplines</p>
          </div>
          <div>
            <p className="font-mono-num text-2xl font-bold text-amber-400">10%</p>
            <p className="text-xs text-slate-400 mt-1">Configurable Platform Fee</p>
          </div>
          <div>
            <p className="font-mono-num text-2xl font-bold text-white">100%</p>
            <p className="text-xs text-slate-400 mt-1">Uncompressed Portfolio Uploads</p>
          </div>
          <div>
            <p className="font-mono-num text-2xl font-bold text-white">C11 + SQLite</p>
            <p className="text-xs text-slate-400 mt-1">Compiled Relational Backend</p>
          </div>
        </div>
      </section>

      {/* Popular Creative Categories */}
      <section className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 space-y-6">
        <div className="flex flex-col sm:flex-row sm:items-end justify-between gap-4">
          <div>
            <h2 className="font-display text-2xl sm:text-3xl font-bold text-white">Popular Creative Categories</h2>
            <p className="text-sm text-slate-400 mt-1">Select a craft to browse specialists and compare project pricing.</p>
          </div>
          <button
            type="button"
            onClick={() => onNavigate('discover')}
            className="text-sm font-medium text-amber-400 hover:text-amber-300 inline-flex items-center gap-1.5 self-start sm:self-auto"
          >
            <span>View all creators</span>
            <ArrowRight className="w-4 h-4" />
          </button>
        </div>

        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
          {CATEGORIES.map((cat, idx) => (
            <button
              key={cat.name}
              type="button"
              onClick={() => onSelectCategory(cat.name)}
              className="glass-panel glass-panel-hover rounded-2xl p-5 text-left flex flex-col justify-between min-h-[124px]"
            >
              <span className="font-mono-num text-xs text-amber-400">0{idx + 1}.</span>
              <div className="mt-3">
                <h3 className="font-display text-base font-semibold text-white">{cat.name}</h3>
                <p className="text-xs text-slate-400 mt-1 leading-relaxed">{cat.desc}</p>
              </div>
            </button>
          ))}
        </div>
      </section>

      {/* Featured Freelancers */}
      <section className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 space-y-6">
        <div className="flex flex-col sm:flex-row sm:items-end justify-between gap-4">
          <div>
            <h2 className="font-display text-2xl sm:text-3xl font-bold text-white">Featured Creative Talent</h2>
            <p className="text-sm text-slate-400 mt-1">Top-rated professionals active on CUTCRAFT.</p>
          </div>
          <button
            type="button"
            onClick={() => onNavigate('discover')}
            className="text-sm font-medium text-amber-400 hover:text-amber-300 inline-flex items-center gap-1.5 self-start sm:self-auto"
          >
            <span>Explore full directory</span>
            <ArrowRight className="w-4 h-4" />
          </button>
        </div>

        {loading ? (
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-5">
            {[1, 2, 3, 4].map((n) => (
              <div key={n} className="glass-panel rounded-2xl h-72 animate-pulse" />
            ))}
          </div>
        ) : (
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-5">
            {featured.map((f) => (
              <FreelancerCard key={f.id} freelancer={f} onSelect={onSelectFreelancer} />
            ))}
          </div>
        )}
      </section>

      {/* Trending Portfolios */}
      <section className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 space-y-6">
        <div>
          <h2 className="font-display text-2xl sm:text-3xl font-bold text-white">Trending Portfolios</h2>
          <p className="text-sm text-slate-400 mt-1">Click any piece to inspect full-resolution frames or play video reels.</p>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-3 gap-5">
          {portfolios.slice(0, 6).map((item, i) => (
            <button
              key={item.id}
              type="button"
              onClick={() => onOpenPortfolio(item)}
              className={`glass-panel glass-panel-hover rounded-2xl overflow-hidden text-left group ${
                i === 0 ? 'md:col-span-2' : ''
              }`}
            >
              <div className={`relative bg-slate-900 overflow-hidden ${i === 0 ? 'h-64 sm:h-72' : 'h-56'}`}>
                <img
                  src={item.thumbnail_path || item.file_path}
                  alt={item.title}
                  referrerPolicy="no-referrer"
                  className="w-full h-full object-cover group-hover:scale-[1.02] transition-transform duration-300"
                />
                <div className="absolute inset-0 bg-gradient-to-t from-[#050814]/90 via-[#050814]/25 to-transparent" />
                {item.media_type === 'video' && (
                  <div className="absolute top-3 right-3 w-10 h-10 rounded-full bg-black/65 border border-white/25 flex items-center justify-center text-amber-400">
                    <Play className="w-4 h-4 fill-amber-400" />
                  </div>
                )}
                <div className="absolute bottom-4 left-4 right-4">
                  <p className="text-xs text-amber-300">
                    {item.category} · {item.creator_name || 'Creator'}
                  </p>
                  <h3 className="font-display text-lg font-semibold text-white mt-0.5">{item.title}</h3>
                </div>
              </div>
            </button>
          ))}
        </div>
      </section>
    </div>
  );
};
