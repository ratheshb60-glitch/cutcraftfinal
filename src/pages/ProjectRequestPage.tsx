import React, { useEffect, useState } from 'react';
import { ArrowLeft, Upload } from 'lucide-react';
import { api, uploadOriginalFile } from '../api/client';
import type { CommissionBreakdown, Freelancer, PageRoute, User } from '../types';

interface ProjectRequestPageProps {
  user: User | null;
  selectedFreelancer: Freelancer | null;
  onNavigate: (page: PageRoute) => void;
}

export const ProjectRequestPage: React.FC<ProjectRequestPageProps> = ({
  user,
  selectedFreelancer,
  onNavigate,
}) => {
  const [freelancers, setFreelancers] = useState<Freelancer[]>([]);
  const [freelancerId, setFreelancerId] = useState<number>(selectedFreelancer?.id || 0);
  const [title, setTitle] = useState('');
  const [description, setDescription] = useState('');
  const [budget, setBudget] = useState(String(selectedFreelancer?.starting_price || 3500));
  const [deadline, setDeadline] = useState('2026-12-15');
  const [attachmentPath, setAttachmentPath] = useState('');
  const [attachmentName, setAttachmentName] = useState('');
  const [commission, setCommission] = useState<CommissionBreakdown | null>(null);
  const [submitting, setSubmitting] = useState(false);
  const [error, setError] = useState('');
  const [success, setSuccess] = useState('');

  useEffect(() => {
    api.getAllFreelancers().then((res) => {
      setFreelancers(res.freelancers);
      if (!freelancerId && res.freelancers.length > 0) {
        setFreelancerId(res.freelancers[0].id);
      }
    });
  }, []);

  useEffect(() => {
    if (selectedFreelancer) {
      setFreelancerId(selectedFreelancer.id);
      setBudget(String(selectedFreelancer.starting_price));
    }
  }, [selectedFreelancer]);

  useEffect(() => {
    const num = Number(budget) || 0;
    api.previewCommission(num).then(setCommission).catch(() => {});
  }, [budget]);

  if (!user) {
    return (
      <div className="max-w-xl mx-auto px-4 py-16 text-center space-y-4">
        <div className="glass-panel rounded-3xl p-8 space-y-4">
          <h1 className="font-display text-2xl font-bold text-white">Sign In to Send a Project Request</h1>
          <p className="text-sm text-slate-400">
            Create a CUTCRAFT client account or sign in to submit briefs and hire creators.
          </p>
          <div className="flex justify-center gap-3">
            <button type="button" onClick={() => onNavigate('login')} className="gold-cta px-5 py-2.5 text-xs rounded-xl">
              Sign In
            </button>
            <button
              type="button"
              onClick={() => onNavigate('register')}
              className="px-5 py-2.5 text-xs rounded-xl border border-white/15 text-white"
            >
              Create Account
            </button>
          </div>
        </div>
      </div>
    );
  }

  const handleAttachment = async (file: File) => {
    try {
      const res = await uploadOriginalFile(file);
      setAttachmentPath(res.file_path);
      setAttachmentName(res.original_filename);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Attachment upload failed.');
    }
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');
    setSuccess('');
    if (user.account_type !== 'client') {
      setError('Only Client accounts can submit hiring requests.');
      return;
    }

    setSubmitting(true);
    try {
      const res = await api.createProjectRequest({
        freelancer_id: freelancerId,
        title: title.trim(),
        description: description.trim(),
        budget: Number(budget),
        deadline,
        attachment_path: attachmentPath,
      });
      setSuccess(
        `Request #${res.id} submitted. Platform commission (${res.commission_rate}%): ₹${res.platform_fee}. Freelancer payout: ₹${res.freelancer_earnings}.`
      );
      setTitle('');
      setDescription('');
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to send request.');
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <div className="max-w-4xl mx-auto px-4 sm:px-6 py-10 space-y-6">
      <button
        type="button"
        onClick={() => onNavigate('discover')}
        className="inline-flex items-center gap-2 text-sm text-slate-300 hover:text-white min-h-[44px]"
      >
        <ArrowLeft className="w-4 h-4" />
        <span>Back to Explore</span>
      </button>

      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        <form onSubmit={handleSubmit} className="lg:col-span-7 glass-panel rounded-3xl p-6 sm:p-8 space-y-4">
          <div>
            <p className="text-xs text-amber-400 font-medium">CUTCRAFT Hiring Brief</p>
            <h1 className="font-display text-2xl font-bold text-white mt-1">Send Project Request</h1>
          </div>

          {error && <div className="p-3 rounded-xl bg-rose-500/10 border border-rose-400/30 text-xs text-rose-200">{error}</div>}
          {success && (
            <div className="p-4 rounded-xl bg-emerald-500/10 border border-emerald-400/30 text-xs text-emerald-200 space-y-2">
              <p>{success}</p>
              <button
                type="button"
                onClick={() => onNavigate('projects')}
                className="text-amber-300 font-semibold hover:underline"
              >
                Track in Project Management →
              </button>
            </div>
          )}

          <div>
            <label className="block text-xs text-slate-300 mb-1">Select Freelancer</label>
            <select
              value={freelancerId}
              onChange={(e) => setFreelancerId(Number(e.target.value))}
              className="glass-input w-full rounded-xl px-4 py-2.5 text-sm"
            >
              {freelancers.map((f) => (
                <option key={f.id} value={f.id} className="bg-slate-900">
                  {f.full_name} — {f.profession} (From ₹{f.starting_price})
                </option>
              ))}
            </select>
          </div>

          <div>
            <label className="block text-xs text-slate-300 mb-1">Project Title</label>
            <input
              type="text"
              required
              value={title}
              onChange={(e) => setTitle(e.target.value)}
              placeholder="e.g. 60-Second Launch Film Edit"
              className="glass-input w-full rounded-xl px-4 py-2.5 text-sm"
            />
          </div>

          <div>
            <label className="block text-xs text-slate-300 mb-1">Brief &amp; Deliverables</label>
            <textarea
              rows={4}
              required
              value={description}
              onChange={(e) => setDescription(e.target.value)}
              placeholder="Describe aspect ratio, runtime, visual mood, and expected files..."
              className="glass-input w-full rounded-xl px-4 py-2.5 text-sm"
            />
          </div>

          <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
            <div>
              <label className="block text-xs text-slate-300 mb-1">Project Budget (₹)</label>
              <input
                type="number"
                min="100"
                required
                value={budget}
                onChange={(e) => setBudget(e.target.value)}
                className="glass-input w-full rounded-xl px-4 py-2.5 text-sm font-mono-num"
              />
            </div>
            <div>
              <label className="block text-xs text-slate-300 mb-1">Target Deadline</label>
              <input
                type="date"
                required
                value={deadline}
                onChange={(e) => setDeadline(e.target.value)}
                className="glass-input w-full rounded-xl px-4 py-2.5 text-sm font-mono-num"
              />
            </div>
          </div>

          <div>
            <label className="block text-xs text-slate-300 mb-1">Reference Attachment (Optional)</label>
            <label className="glass-input rounded-xl px-4 py-3 text-xs text-slate-300 cursor-pointer flex items-center gap-2">
              <Upload className="w-4 h-4 text-amber-400" />
              <span className="truncate">{attachmentName || 'Attach moodboard, script PDF, or sample frame'}</span>
              <input
                type="file"
                accept=".jpg,.jpeg,.png,.webp,.pdf,.mp4"
                className="hidden"
                onChange={(e) => {
                  const f = e.target.files?.[0];
                  if (f) handleAttachment(f);
                }}
              />
            </label>
          </div>

          <button
            type="submit"
            disabled={submitting}
            className="gold-cta w-full py-3.5 text-sm rounded-xl min-h-[44px] disabled:opacity-60"
          >
            {submitting ? 'Submitting Request...' : 'Submit Project Request'}
          </button>
        </form>

        {/* Live Commission Breakdown from commission.c */}
        <aside className="lg:col-span-5 glass-panel rounded-3xl p-6 space-y-5 h-fit">
          <div>
            <h2 className="font-display text-lg font-bold text-white">Commission &amp; Payout Breakdown</h2>
            <p className="text-xs text-slate-400 mt-1">Calculated directly by the CUTCRAFT C commission module.</p>
          </div>

          <div className="space-y-3 text-sm border-t border-b border-white/[0.08] py-4">
            <div className="flex items-center justify-between">
              <span className="text-slate-400">Project Budget</span>
              <span className="font-mono-num font-semibold text-white">
                ₹{(commission?.budget || Number(budget) || 0).toLocaleString('en-IN')}
              </span>
            </div>
            <div className="flex items-center justify-between">
              <span className="text-slate-400">Platform Commission ({commission?.commission_rate ?? 10}%)</span>
              <span className="font-mono-num text-slate-300">
                ₹{(commission?.platform_earnings || 0).toLocaleString('en-IN')}
              </span>
            </div>
            <div className="flex items-center justify-between pt-2 border-t border-white/[0.06]">
              <span className="text-slate-200 font-medium">Freelancer Earnings</span>
              <span className="font-mono-num text-lg font-bold text-amber-400">
                ₹{(commission?.freelancer_earnings || 0).toLocaleString('en-IN')}
              </span>
            </div>
          </div>

          <p className="text-xs text-slate-400 leading-relaxed">
            Payment is handled via CUTCRAFT Simulated Escrow (v1) once the freelancer accepts your brief. No real currency is charged in this version.
          </p>
        </aside>
      </div>
    </div>
  );
};
