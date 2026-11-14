import React from 'react';
import { Star } from 'lucide-react';
import type { Freelancer } from '../types';

interface FreelancerCardProps {
  freelancer: Freelancer;
  onSelect: (id: number) => void;
}

export const FreelancerCard: React.FC<FreelancerCardProps> = ({ freelancer, onSelect }) => {
  const skills = freelancer.skills
    .split(',')
    .map((s) => s.trim())
    .filter(Boolean)
    .slice(0, 3);

  return (
    <article
      onClick={() => onSelect(freelancer.id)}
      role="button"
      tabIndex={0}
      onKeyDown={(e) => {
        if (e.key === 'Enter' || e.key === ' ') {
          e.preventDefault();
          onSelect(freelancer.id);
        }
      }}
      className="glass-panel glass-panel-hover rounded-2xl overflow-hidden cursor-pointer flex flex-col justify-between focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-amber-400"
    >
      <div>
        <div className="relative h-44 bg-slate-900 overflow-hidden">
          <img
            src={freelancer.cover_url || freelancer.avatar_url}
            alt={`${freelancer.full_name} creative work`}
            referrerPolicy="no-referrer"
            className="w-full h-full object-cover"
          />
          <div className="absolute inset-0 bg-gradient-to-t from-[#050814]/90 via-[#050814]/25 to-transparent" />
          <div className="absolute bottom-3 left-4 right-4 flex items-end gap-3">
            <img
              src={freelancer.avatar_url}
              alt={freelancer.full_name}
              referrerPolicy="no-referrer"
              className="w-11 h-11 rounded-xl object-cover border border-white/25 shrink-0 bg-slate-800"
            />
            <div className="min-w-0">
              <h3 className="font-display text-base font-semibold text-white truncate">{freelancer.full_name}</h3>
              <p className="text-xs text-amber-300 truncate">{freelancer.profession}</p>
            </div>
          </div>
        </div>

        <div className="p-4 space-y-3">
          <p className="text-sm text-slate-300 line-clamp-2 leading-relaxed">{freelancer.bio}</p>
          <p className="text-xs text-slate-400 truncate">
            {skills.join(' · ') || freelancer.location}
          </p>
        </div>
      </div>

      <div className="px-4 py-3 border-t border-white/[0.07] flex items-center justify-between gap-2 text-xs">
        <div className="flex items-center gap-1.5 text-slate-200 font-mono-num">
          <Star className="w-3.5 h-3.5 text-amber-400 fill-amber-400" />
          <span>{freelancer.rating_avg.toFixed(1)}</span>
          <span className="text-slate-500">·</span>
          <span className="text-slate-400">{freelancer.completed_count} projects</span>
        </div>
        <div className="font-mono-num text-right">
          <span className="text-slate-400 mr-1">From</span>
          <span className="text-sm font-semibold text-white">₹{freelancer.starting_price.toLocaleString('en-IN')}</span>
        </div>
      </div>
    </article>
  );
};
