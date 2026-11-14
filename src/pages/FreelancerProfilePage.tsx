import React, { useEffect, useState } from 'react';
import { ArrowLeft, MessageSquare, Play, Star } from 'lucide-react';
import { api } from '../api/client';
import type { Freelancer, PortfolioItem, Review, User } from '../types';

interface FreelancerProfilePageProps {
  freelancerId: number;
  currentUser: User | null;
  onBack: () => void;
  onHireNow: (freelancer: Freelancer) => void;
  onMessageCreator: (userId: number) => void;
  onOpenPortfolio: (item: PortfolioItem) => void;
}

export const FreelancerProfilePage: React.FC<FreelancerProfilePageProps> = ({
  freelancerId,
  currentUser,
  onBack,
  onHireNow,
  onMessageCreator,
  onOpenPortfolio,
}) => {
  const [freelancer, setFreelancer] = useState<Freelancer | null>(null);
  const [portfolio, setPortfolio] = useState<PortfolioItem[]>([]);
  const [reviews, setReviews] = useState<Review[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');

  useEffect(() => {
    let active = true;
    setLoading(true);
    api
      .getFreelancerDetail(freelancerId)
      .then((res) => {
        if (!active) return;
        setFreelancer(res.freelancer);
        setPortfolio(res.portfolio);
        setReviews(res.reviews);
      })
      .catch((err) => {
        if (active) setError(err instanceof Error ? err.message : 'Failed to load profile.');
      })
      .finally(() => {
        if (active) setLoading(false);
      });
    return () => {
      active = false;
    };
  }, [freelancerId]);

  if (loading) {
    return (
      <div className="max-w-6xl mx-auto px-4 py-12">
        <div className="glass-panel rounded-3xl h-64 animate-pulse" />
      </div>
    );
  }

  if (error || !freelancer) {
    return (
      <div className="max-w-4xl mx-auto px-4 py-12 space-y-4">
        <p className="text-rose-300">{error || 'Freelancer not found.'}</p>
        <button type="button" onClick={onBack} className="text-sm text-amber-400 hover:underline">
          Back to Explore
        </button>
      </div>
    );
  }

  const skillsList = freelancer.skills
    .split(',')
    .map((s) => s.trim())
    .filter(Boolean);

  return (
    <div className="max-w-6xl mx-auto px-4 sm:px-6 lg:px-8 py-8 space-y-10">
      <button
        type="button"
        onClick={onBack}
        className="inline-flex items-center gap-2 text-sm text-slate-300 hover:text-white min-h-[44px]"
      >
        <ArrowLeft className="w-4 h-4" />
        <span>Back to Explore</span>
      </button>

      {/* Profile Header */}
      <div className="glass-panel rounded-3xl p-6 sm:p-8 flex flex-col lg:flex-row lg:items-center justify-between gap-8">
        <div className="flex flex-col sm:flex-row items-start sm:items-center gap-5">
          <img
            src={freelancer.avatar_url}
            alt={freelancer.full_name}
            referrerPolicy="no-referrer"
            className="w-24 h-24 rounded-2xl object-cover border border-white/20 shrink-0"
          />
          <div className="space-y-2">
            <h1 className="font-display text-2xl sm:text-3xl font-bold text-white">{freelancer.full_name}</h1>
            <p className="text-sm text-amber-300 font-medium">
              {freelancer.profession} · {freelancer.location}
            </p>
            <div className="flex flex-wrap items-center gap-4 text-xs text-slate-300 font-mono-num">
              <span className="inline-flex items-center gap-1">
                <Star className="w-3.5 h-3.5 text-amber-400 fill-amber-400" />
                {freelancer.rating_avg.toFixed(1)} ({freelancer.review_count} reviews)
              </span>
              <span className="text-slate-600">·</span>
              <span>{freelancer.completed_count} completed projects</span>
              <span className="text-slate-600">·</span>
              <span>Starting at ₹{freelancer.starting_price.toLocaleString('en-IN')}</span>
            </div>
          </div>
        </div>

        <div className="flex flex-wrap items-center gap-3 shrink-0">
          {currentUser?.id !== freelancer.user_id && (
            <>
              <button
                type="button"
                onClick={() => onMessageCreator(freelancer.user_id)}
                className="px-5 py-3 text-sm font-medium rounded-xl border border-white/15 bg-white/[0.04] text-white hover:border-amber-400/50 inline-flex items-center gap-2 min-h-[44px]"
              >
                <MessageSquare className="w-4 h-4" />
                <span>Message</span>
              </button>
              <button
                type="button"
                onClick={() => onHireNow(freelancer)}
                className="gold-cta px-6 py-3 text-sm rounded-xl min-h-[44px]"
              >
                Hire Now
              </button>
            </>
          )}
        </div>
      </div>

      {/* Bio & Skills */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        <div className="lg:col-span-2 glass-panel rounded-2xl p-6 space-y-3">
          <h2 className="font-display text-lg font-semibold text-white">About the Creator</h2>
          <p className="text-sm text-slate-300 leading-relaxed">{freelancer.bio || 'Creative specialist on CUTCRAFT.'}</p>
        </div>
        <div className="glass-panel rounded-2xl p-6 space-y-3">
          <h2 className="font-display text-lg font-semibold text-white">Core Tools &amp; Skills</h2>
          <p className="text-sm text-slate-300 leading-relaxed">
            {skillsList.length > 0 ? skillsList.join(' · ') : freelancer.profession}
          </p>
        </div>
      </div>

      {/* Portfolio Gallery */}
      <section className="space-y-4">
        <div className="flex items-center justify-between">
          <h2 className="font-display text-2xl font-bold text-white">Portfolio Gallery</h2>
          <span className="font-mono-num text-xs text-slate-400">{portfolio.length} items</span>
        </div>

        {portfolio.length === 0 ? (
          <div className="glass-panel rounded-2xl p-8 text-center text-sm text-slate-400">
            No portfolio works uploaded yet.
          </div>
        ) : (
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-5">
            {portfolio.map((item) => (
              <button
                key={item.id}
                type="button"
                onClick={() => onOpenPortfolio(item)}
                className="glass-panel glass-panel-hover rounded-2xl overflow-hidden text-left group"
              >
                <div className="relative h-52 bg-slate-900 overflow-hidden">
                  <img
                    src={item.thumbnail_path || item.file_path}
                    alt={item.title}
                    referrerPolicy="no-referrer"
                    className="w-full h-full object-cover"
                  />
                  <div className="absolute inset-0 bg-gradient-to-t from-[#050814]/85 via-transparent to-transparent" />
                  {item.media_type === 'video' && (
                    <div className="absolute top-3 right-3 w-9 h-9 rounded-full bg-black/70 border border-white/20 flex items-center justify-center text-amber-400">
                      <Play className="w-4 h-4 fill-amber-400" />
                    </div>
                  )}
                  <div className="absolute bottom-3 left-4 right-4">
                    <p className="text-xs text-amber-300">{item.category} · {item.media_type === 'video' ? 'Video' : 'Photo'}</p>
                    <h3 className="font-display text-base font-semibold text-white truncate">{item.title}</h3>
                  </div>
                </div>
              </button>
            ))}
          </div>
        )}
      </section>

      {/* Client Reviews */}
      <section className="space-y-4">
        <h2 className="font-display text-2xl font-bold text-white">Client Reviews</h2>
        {reviews.length === 0 ? (
          <div className="glass-panel rounded-2xl p-6 text-sm text-slate-400">
            No reviews submitted yet.
          </div>
        ) : (
          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            {reviews.map((rev) => (
              <div key={rev.id} className="glass-panel rounded-2xl p-5 space-y-3">
                <div className="flex items-center justify-between gap-2">
                  <div>
                    <p className="text-sm font-semibold text-white">{rev.client_name}</p>
                    <p className="text-xs text-slate-400">{rev.project_title}</p>
                  </div>
                  <div className="flex items-center gap-1 font-mono-num text-xs text-amber-400">
                    <Star className="w-3.5 h-3.5 fill-amber-400" />
                    <span>{rev.rating}.0</span>
                  </div>
                </div>
                <p className="text-sm text-slate-300 leading-relaxed">{rev.comment}</p>
              </div>
            ))}
          </div>
        )}
      </section>
    </div>
  );
};
