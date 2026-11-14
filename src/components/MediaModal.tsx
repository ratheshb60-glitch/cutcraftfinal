import React, { useEffect } from 'react';
import { X } from 'lucide-react';
import type { PortfolioItem } from '../types';

interface MediaModalProps {
  item: PortfolioItem | null;
  onClose: () => void;
  onOpenCreator?: (freelancerId: number) => void;
}

export const MediaModal: React.FC<MediaModalProps> = ({ item, onClose, onOpenCreator }) => {
  useEffect(() => {
    if (!item) return;
    const onKey = (e: KeyboardEvent) => {
      if (e.key === 'Escape') onClose();
    };
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [item, onClose]);

  if (!item) return null;

  return (
    <div
      className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/85 backdrop-blur-md"
      role="dialog"
      aria-modal="true"
      aria-label={item.title}
      onClick={onClose}
    >
      <div
        className="glass-panel rounded-2xl max-w-4xl w-full overflow-hidden"
        onClick={(e) => e.stopPropagation()}
      >
        <div className="flex items-center justify-between px-5 py-4 border-b border-white/[0.08]">
          <div>
            <h3 className="font-display text-lg font-semibold text-white">{item.title}</h3>
            <p className="text-xs text-slate-400 mt-0.5">
              <span>{item.category}</span>
              <span className="mx-2" aria-hidden="true">·</span>
              <span>{item.media_type === 'video' ? 'Original Video Stream' : 'Original Photo Asset'}</span>
              {item.creator_name && (
                <>
                  <span className="mx-2" aria-hidden="true">·</span>
                  <span>By {item.creator_name}</span>
                </>
              )}
            </p>
          </div>
          <button
            type="button"
            onClick={onClose}
            aria-label="Close preview"
            className="min-h-[44px] min-w-[44px] flex items-center justify-center rounded-lg text-slate-300 hover:text-white hover:bg-white/10 transition-colors"
          >
            <X className="w-5 h-5" />
          </button>
        </div>

        <div className="bg-black flex items-center justify-center max-h-[65vh] overflow-hidden">
          {item.media_type === 'video' ? (
            <video
              src={item.file_path}
              poster={item.thumbnail_path || undefined}
              controls
              playsInline
              className="w-full max-h-[65vh] object-contain"
            />
          ) : (
            <img
              src={item.file_path}
              alt={item.title}
              referrerPolicy="no-referrer"
              className="w-full max-h-[65vh] object-contain"
            />
          )}
        </div>

        <div className="px-5 py-4 flex flex-col sm:flex-row sm:items-center justify-between gap-4 border-t border-white/[0.08]">
          <div className="space-y-1">
            <p className="text-sm text-slate-300">{item.description || 'Original creative portfolio upload.'}</p>
            <p className="text-xs text-slate-500">
              Storage: {item.storage_provider || 'local_original'} · Uncompressed source preserved · Cloudflare R2 &amp; Mux adaptive pipeline ready
            </p>
          </div>
          {onOpenCreator && item.freelancer_id && (
            <button
              type="button"
              onClick={() => {
                onClose();
                onOpenCreator(item.freelancer_id);
              }}
              className="gold-cta px-4 py-2.5 text-xs rounded-lg whitespace-nowrap shrink-0"
            >
              View Creator Profile
            </button>
          )}
        </div>
      </div>
    </div>
  );
};
