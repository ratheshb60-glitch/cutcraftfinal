import React, { useEffect, useState } from 'react';
import { Search } from 'lucide-react';
import { api } from '../api/client';
import type { Freelancer } from '../types';
import { FreelancerCard } from '../components/FreelancerCard';

interface DiscoveryPageProps {
  initialCategory?: string;
  onSelectFreelancer: (id: number) => void;
}

const FILTER_CATEGORIES = [
  'All',
  'Video Editors',
  'Photo Editors',
  'Photographers',
  'Videographers',
  'Graphic Designers',
  'Motion Graphics Designers',
  'Thumbnail Designers',
];

export const DiscoveryPage: React.FC<DiscoveryPageProps> = ({ initialCategory = 'All', onSelectFreelancer }) => {
  const [query, setQuery] = useState('');
  const [category, setCategory] = useState(initialCategory);
  const [minPrice, setMinPrice] = useState('');
  const [maxPrice, setMaxPrice] = useState('');
  const [minRating, setMinRating] = useState('');
  const [sort, setSort] = useState('rating');
  const [results, setResults] = useState<Freelancer[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');

  useEffect(() => {
    setCategory(initialCategory || 'All');
  }, [initialCategory]);

  useEffect(() => {
    let active = true;
    const timer = setTimeout(async () => {
      setLoading(true);
      setError('');
      try {
        const res = await api.searchFreelancers({
          q: query.trim(),
          category: category === 'All' ? '' : category,
          min_price: minPrice,
          max_price: maxPrice,
          min_rating: minRating,
          sort,
        });
        if (active) setResults(res.freelancers);
      } catch (err) {
        if (active) setError(err instanceof Error ? err.message : 'Search failed.');
      } finally {
        if (active) setLoading(false);
      }
    }, 180);
    return () => {
      active = false;
      clearTimeout(timer);
    };
  }, [query, category, minPrice, maxPrice, minRating, sort]);

  const resetFilters = () => {
    setQuery('');
    setCategory('All');
    setMinPrice('');
    setMaxPrice('');
    setMinRating('');
    setSort('rating');
  };

  return (
    <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-10 space-y-8">
      <div className="flex flex-col md:flex-row md:items-end justify-between gap-4">
        <div>
          <h1 className="font-display text-3xl sm:text-4xl font-bold text-white">Explore CUTCRAFT Creators</h1>
          <p className="text-sm text-slate-400 mt-1">
            Search verified freelancers by craft, price range, rating, and completed deliveries.
          </p>
        </div>
        <p className="font-mono-num text-xs text-slate-400">{results.length} matching specialists</p>
      </div>

      {/* Search & Filter Panel */}
      <div className="glass-panel rounded-2xl p-5 space-y-5">
        <div className="grid grid-cols-1 lg:grid-cols-12 gap-4">
          <div className="lg:col-span-5 relative">
            <Search className="w-4 h-4 text-slate-400 absolute left-3.5 top-1/2 -translate-y-1/2" />
            <input
              type="text"
              value={query}
              onChange={(e) => setQuery(e.target.value)}
              placeholder="Search name, skill (DaVinci, Photoshop, Cinema 4D)..."
              className="glass-input w-full rounded-xl pl-10 pr-4 py-2.5 text-sm"
            />
          </div>

          <div className="lg:col-span-2">
            <input
              type="number"
              min="0"
              value={minPrice}
              onChange={(e) => setMinPrice(e.target.value)}
              placeholder="Min ₹"
              className="glass-input w-full rounded-xl px-3.5 py-2.5 text-sm font-mono-num"
            />
          </div>

          <div className="lg:col-span-2">
            <input
              type="number"
              min="0"
              value={maxPrice}
              onChange={(e) => setMaxPrice(e.target.value)}
              placeholder="Max ₹"
              className="glass-input w-full rounded-xl px-3.5 py-2.5 text-sm font-mono-num"
            />
          </div>

          <div className="lg:col-span-3 flex gap-2">
            <select
              value={minRating}
              onChange={(e) => setMinRating(e.target.value)}
              aria-label="Minimum rating"
              className="glass-input flex-1 rounded-xl px-3 py-2.5 text-sm"
            >
              <option value="" className="bg-slate-900">Any rating</option>
              <option value="4.5" className="bg-slate-900">4.5+ stars</option>
              <option value="4.8" className="bg-slate-900">4.8+ stars</option>
            </select>
            <select
              value={sort}
              onChange={(e) => setSort(e.target.value)}
              aria-label="Sort results"
              className="glass-input flex-1 rounded-xl px-3 py-2.5 text-sm"
            >
              <option value="rating" className="bg-slate-900">Top Rated</option>
              <option value="price_asc" className="bg-slate-900">Price: Low-High</option>
              <option value="price_desc" className="bg-slate-900">Price: High-Low</option>
              <option value="completed" className="bg-slate-900">Most Projects</option>
            </select>
          </div>
        </div>

        <div className="flex flex-wrap items-center gap-2">
          {FILTER_CATEGORIES.map((cat) => (
            <button
              key={cat}
              type="button"
              onClick={() => setCategory(cat)}
              className={`px-3.5 py-2 text-xs font-medium rounded-lg transition-colors whitespace-nowrap shrink-0 min-h-[38px] ${
                category === cat
                  ? 'bg-amber-500 text-slate-950 font-semibold'
                  : 'bg-white/[0.04] text-slate-300 border border-white/[0.08] hover:text-white'
              }`}
            >
              {cat}
            </button>
          ))}
        </div>
      </div>

      {error && (
        <div className="p-4 rounded-xl bg-rose-500/10 border border-rose-400/30 text-sm text-rose-200">{error}</div>
      )}

      {loading ? (
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-6">
          {[1, 2, 3, 4, 5, 6].map((n) => (
            <div key={n} className="glass-panel rounded-2xl h-72 animate-pulse" />
          ))}
        </div>
      ) : results.length === 0 ? (
        <div className="glass-panel rounded-2xl p-10 text-center space-y-4">
          <p className="font-display text-xl font-semibold text-white">No creators match those filters</p>
          <p className="text-sm text-slate-400">Try widening your budget range or clearing category keywords.</p>
          <button
            type="button"
            onClick={resetFilters}
            className="gold-cta px-5 py-2.5 text-xs rounded-xl"
          >
            Reset Filters
          </button>
        </div>
      ) : (
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-6">
          {results.map((f) => (
            <FreelancerCard key={f.id} freelancer={f} onSelect={onSelectFreelancer} />
          ))}
        </div>
      )}
    </div>
  );
};
