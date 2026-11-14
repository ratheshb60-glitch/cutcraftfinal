import React, { useEffect, useState } from 'react';
import { Check, CreditCard, Star, X } from 'lucide-react';
import { api } from '../api/client';
import type { Project, ProjectRequest, User } from '../types';

interface ProjectsPageProps {
  user: User;
  onMessageUser: (peerUserId: number) => void;
}

type TabFilter = 'all' | 'pending' | 'accepted' | 'in_progress' | 'completed' | 'rejected';

export const ProjectsPage: React.FC<ProjectsPageProps> = ({ user, onMessageUser }) => {
  const [tab, setTab] = useState<TabFilter>('all');
  const [requests, setRequests] = useState<ProjectRequest[]>([]);
  const [projects, setProjects] = useState<Project[]>([]);
  const [loading, setLoading] = useState(true);

  /* Simulated Payment Modal */
  const [payTarget, setPayTarget] = useState<Project | null>(null);
  const [payMethod, setPayMethod] = useState('Simulated UPI / Escrow (v1)');
  const [payNotice, setPayNotice] = useState('');

  /* Review Modal */
  const [reviewTarget, setReviewTarget] = useState<Project | null>(null);
  const [rating, setRating] = useState(5);
  const [comment, setComment] = useState('');

  const loadAll = async () => {
    setLoading(true);
    try {
      const [rRes, pRes] = await Promise.all([api.getProjectRequests(), api.getProjects()]);
      setRequests(rRes.requests);
      setProjects(pRes.projects);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadAll();
  }, [user.id]);

  const handleRespond = async (id: number, action: 'accept' | 'reject') => {
    await api.respondProjectRequest(id, action);
    await loadAll();
  };

  const handleStatusChange = async (id: number, status: string, progress_percent: number) => {
    await api.updateProjectStatus(id, { status, progress_percent });
    await loadAll();
  };

  const handleSimulatedPay = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!payTarget) return;
    const res = await api.payProjectSimulated(payTarget.id, payMethod);
    setPayNotice(`${res.notice} Ref: ${res.reference_code}`);
    setPayTarget(null);
    await loadAll();
  };

  const handleReviewSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!reviewTarget) return;
    await api.submitReview(reviewTarget.id, { rating, comment });
    setReviewTarget(null);
    setComment('');
    await loadAll();
  };

  const pendingReqs = requests.filter((r) => r.status === 'pending');
  const rejectedReqs = requests.filter((r) => r.status === 'rejected');
  const filteredProjects = projects.filter((p) => {
    if (tab === 'all') return true;
    if (tab === 'accepted' || tab === 'in_progress' || tab === 'completed') return p.status === tab;
    return false;
  });

  const tabs: { id: TabFilter; label: string }[] = [
    { id: 'all', label: 'All Activity' },
    { id: 'pending', label: `Pending (${pendingReqs.length})` },
    { id: 'accepted', label: 'Accepted' },
    { id: 'in_progress', label: 'In Progress' },
    { id: 'completed', label: 'Completed' },
    { id: 'rejected', label: `Rejected (${rejectedReqs.length})` },
  ];

  return (
    <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-10 space-y-8">
      <div>
        <h1 className="font-display text-3xl font-bold text-white">CUTCRAFT Project Management</h1>
        <p className="text-sm text-slate-400 mt-1">
          Track hiring requests, project milestones, commission breakdowns, and simulated escrow payments.
        </p>
      </div>

      {payNotice && (
        <div className="p-4 rounded-xl bg-emerald-500/10 border border-emerald-400/30 text-sm text-emerald-200 flex items-center justify-between gap-4">
          <span>{payNotice}</span>
          <button type="button" onClick={() => setPayNotice('')} className="text-xs underline">
            Dismiss
          </button>
        </div>
      )}

      {/* Segmented Filter Tabs */}
      <div className="flex flex-wrap gap-2">
        {tabs.map((t) => (
          <button
            key={t.id}
            type="button"
            onClick={() => setTab(t.id)}
            className={`px-4 py-2 text-xs font-medium rounded-xl transition-colors whitespace-nowrap min-h-[40px] ${
              tab === t.id
                ? 'bg-amber-500 text-slate-950 font-semibold'
                : 'bg-white/[0.04] text-slate-300 border border-white/[0.08] hover:text-white'
            }`}
          >
            {t.label}
          </button>
        ))}
      </div>

      {loading ? (
        <div className="glass-panel rounded-2xl h-48 animate-pulse" />
      ) : (
        <div className="space-y-8">
          {/* Pending Requests */}
          {(tab === 'all' || tab === 'pending') && pendingReqs.length > 0 && (
            <section className="space-y-3">
              <h2 className="font-display text-lg font-semibold text-white">Pending Project Requests</h2>
              <div className="grid grid-cols-1 gap-4">
                {pendingReqs.map((r) => (
                  <div key={r.id} className="glass-panel rounded-2xl p-5 flex flex-col md:flex-row md:items-center justify-between gap-4">
                    <div className="space-y-1">
                      <p className="text-xs text-amber-400">
                        Pending Request · {user.account_type === 'client' ? `To ${r.freelancer_name}` : `From ${r.client_name}`} · Due {r.deadline}
                      </p>
                      <h3 className="font-display text-lg font-semibold text-white">{r.title}</h3>
                      <p className="text-sm text-slate-300">{r.description}</p>
                      <p className="text-xs text-slate-400 font-mono-num">
                        Budget: ₹{r.budget.toLocaleString('en-IN')} · Fee ({r.commission_rate}%): ₹{r.platform_fee.toLocaleString('en-IN')} · Payout: ₹{r.freelancer_earnings.toLocaleString('en-IN')}
                      </p>
                    </div>
                    {user.account_type === 'freelancer' && (
                      <div className="flex gap-2 shrink-0">
                        <button
                          type="button"
                          onClick={() => handleRespond(r.id, 'accept')}
                          className="gold-cta px-4 py-2 text-xs rounded-xl inline-flex items-center gap-1 min-h-[40px]"
                        >
                          <Check className="w-4 h-4" />
                          <span>Accept</span>
                        </button>
                        <button
                          type="button"
                          onClick={() => handleRespond(r.id, 'reject')}
                          className="px-4 py-2 text-xs rounded-xl border border-rose-400/30 text-rose-200 inline-flex items-center gap-1 min-h-[40px]"
                        >
                          <X className="w-4 h-4" />
                          <span>Reject</span>
                        </button>
                      </div>
                    )}
                  </div>
                ))}
              </div>
            </section>
          )}

          {/* Active & Completed Projects */}
          {(tab !== 'pending' && tab !== 'rejected') && (
            <section className="space-y-3">
              <h2 className="font-display text-lg font-semibold text-white">Projects ({filteredProjects.length})</h2>
              {filteredProjects.length === 0 ? (
                <div className="glass-panel rounded-2xl p-8 text-center text-sm text-slate-400">
                  No projects in this status yet.
                </div>
              ) : (
                <div className="grid grid-cols-1 gap-4">
                  {filteredProjects.map((p) => {
                    const peerUserId = user.account_type === 'client' ? p.freelancer_user_id : p.client_id;
                    return (
                      <div key={p.id} className="glass-panel rounded-2xl p-5 sm:p-6 space-y-4">
                        <div className="flex flex-col lg:flex-row lg:items-center justify-between gap-4">
                          <div className="space-y-1">
                            <p className="text-xs text-amber-300">
                              Status: {p.status.replace('_', ' ')} ·{' '}
                              {user.account_type === 'client' ? `${p.freelancer_name} (${p.freelancer_profession})` : `Client: ${p.client_name}`} · Deadline: {p.deadline}
                            </p>
                            <h3 className="font-display text-xl font-semibold text-white">{p.title}</h3>
                            <p className="text-sm text-slate-300">{p.description}</p>
                          </div>

                          <div className="flex flex-wrap items-center gap-2 shrink-0">
                            <button
                              type="button"
                              onClick={() => onMessageUser(peerUserId)}
                              className="px-4 py-2 text-xs rounded-xl border border-white/15 bg-white/[0.04] text-white hover:border-amber-400/50 min-h-[40px]"
                            >
                              Open Chat
                            </button>

                            {user.account_type === 'client' && p.payment_status === 'unpaid' && p.status !== 'cancelled' && (
                              <button
                                type="button"
                                onClick={() => setPayTarget(p)}
                                className="gold-cta px-4 py-2 text-xs rounded-xl inline-flex items-center gap-1.5 min-h-[40px]"
                              >
                                <CreditCard className="w-4 h-4" />
                                <span>Simulated Pay</span>
                              </button>
                            )}

                            {user.account_type === 'client' && p.status === 'completed' && p.has_review === 0 && (
                              <button
                                type="button"
                                onClick={() => setReviewTarget(p)}
                                className="px-4 py-2 text-xs rounded-xl bg-amber-500/20 border border-amber-400/40 text-amber-200 min-h-[40px]"
                              >
                                Leave Review
                              </button>
                            )}
                          </div>
                        </div>

                        {/* Progress + Commission Breakdown */}
                        <div className="pt-3 border-t border-white/[0.08] grid grid-cols-1 md:grid-cols-12 gap-4 items-center">
                          <div className="md:col-span-6 space-y-1.5">
                            <div className="flex justify-between text-xs text-slate-400">
                              <span>Delivery Progress</span>
                              <span className="font-mono-num text-white">{p.progress_percent}%</span>
                            </div>
                            <div className="w-full h-2 rounded-full bg-white/10 overflow-hidden">
                              <div className="h-full bg-amber-400" style={{ width: `${p.progress_percent}%` }} />
                            </div>
                            {user.account_type === 'freelancer' && p.status !== 'completed' && p.status !== 'cancelled' && (
                              <div className="flex flex-wrap gap-2 pt-2">
                                <button
                                  type="button"
                                  onClick={() => handleStatusChange(p.id, 'in_progress', 50)}
                                  className="px-3 py-1.5 text-xs rounded-lg bg-white/[0.06] text-slate-200 hover:text-white"
                                >
                                  Mark In Progress (50%)
                                </button>
                                <button
                                  type="button"
                                  onClick={() => handleStatusChange(p.id, 'completed', 100)}
                                  className="px-3 py-1.5 text-xs rounded-lg bg-emerald-500/20 text-emerald-200 border border-emerald-400/30"
                                >
                                  Mark Completed (100%)
                                </button>
                              </div>
                            )}
                          </div>

                          <div className="md:col-span-6 grid grid-cols-2 sm:grid-cols-4 gap-3 text-xs font-mono-num">
                            <div>
                              <p className="text-slate-400 font-sans">Budget</p>
                              <p className="text-white font-semibold">₹{p.budget.toLocaleString('en-IN')}</p>
                            </div>
                            <div>
                              <p className="text-slate-400 font-sans">Fee ({p.commission_rate}%)</p>
                              <p className="text-slate-300">₹{p.platform_fee.toLocaleString('en-IN')}</p>
                            </div>
                            <div>
                              <p className="text-slate-400 font-sans">Payout</p>
                              <p className="text-amber-300 font-semibold">₹{p.freelancer_earnings.toLocaleString('en-IN')}</p>
                            </div>
                            <div>
                              <p className="text-slate-400 font-sans">Payment</p>
                              <p className={p.payment_status === 'simulated_paid' ? 'text-emerald-300' : 'text-amber-200'}>
                                {p.payment_status === 'simulated_paid' ? 'Simulated Paid' : 'Unpaid'}
                              </p>
                            </div>
                          </div>
                        </div>
                      </div>
                    );
                  })}
                </div>
              )}
            </section>
          )}

          {/* Rejected Requests */}
          {(tab === 'all' || tab === 'rejected') && rejectedReqs.length > 0 && (
            <section className="space-y-3">
              <h2 className="font-display text-lg font-semibold text-white">Rejected Requests</h2>
              <div className="grid grid-cols-1 gap-3">
                {rejectedReqs.map((r) => (
                  <div key={r.id} className="glass-panel rounded-2xl p-4 flex items-center justify-between gap-4 opacity-80">
                    <div>
                      <h3 className="font-display text-base font-semibold text-white">{r.title}</h3>
                      <p className="text-xs text-slate-400">
                        {r.freelancer_name} · Budget ₹{r.budget.toLocaleString('en-IN')} · Rejected
                      </p>
                    </div>
                  </div>
                ))}
              </div>
            </section>
          )}
        </div>
      )}

      {/* Simulated Payment Modal */}
      {payTarget && (
        <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/80 backdrop-blur-md">
          <form onSubmit={handleSimulatedPay} className="glass-panel rounded-3xl max-w-md w-full p-6 space-y-4">
            <h3 className="font-display text-xl font-bold text-white">Simulated Escrow Payment (v1)</h3>
            <p className="text-xs text-amber-300">
              Demonstration mode: records a simulated transaction in SQLite. No real money is transferred.
            </p>
            <div className="space-y-2 text-sm border-y border-white/[0.08] py-3 font-mono-num">
              <div className="flex justify-between">
                <span className="font-sans text-slate-400">Project</span>
                <span className="font-sans text-white">{payTarget.title}</span>
              </div>
              <div className="flex justify-between">
                <span className="font-sans text-slate-400">Gross Budget</span>
                <span className="text-white">₹{payTarget.budget.toLocaleString('en-IN')}</span>
              </div>
              <div className="flex justify-between">
                <span className="font-sans text-slate-400">Platform Commission ({payTarget.commission_rate}%)</span>
                <span className="text-slate-300">₹{payTarget.platform_fee.toLocaleString('en-IN')}</span>
              </div>
              <div className="flex justify-between font-bold">
                <span className="font-sans text-slate-200">Freelancer Earnings</span>
                <span className="text-amber-400">₹{payTarget.freelancer_earnings.toLocaleString('en-IN')}</span>
              </div>
            </div>

            <div>
              <label className="block text-xs text-slate-300 mb-1">Simulated Payment Method</label>
              <select
                value={payMethod}
                onChange={(e) => setPayMethod(e.target.value)}
                className="glass-input w-full rounded-xl px-3.5 py-2.5 text-sm"
              >
                <option value="Simulated UPI / Escrow (v1)" className="bg-slate-900">Simulated UPI / Escrow (v1)</option>
                <option value="Simulated Card Checkout (v1)" className="bg-slate-900">Simulated Card Checkout (v1)</option>
              </select>
            </div>

            <div className="flex justify-end gap-3 pt-2">
              <button
                type="button"
                onClick={() => setPayTarget(null)}
                className="px-4 py-2 text-xs text-slate-300"
              >
                Cancel
              </button>
              <button type="submit" className="gold-cta px-5 py-2.5 text-xs rounded-xl">
                Confirm Simulated Payment
              </button>
            </div>
          </form>
        </div>
      )}

      {/* Review Modal */}
      {reviewTarget && (
        <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/80 backdrop-blur-md">
          <form onSubmit={handleReviewSubmit} className="glass-panel rounded-3xl max-w-md w-full p-6 space-y-4">
            <h3 className="font-display text-xl font-bold text-white">Review {reviewTarget.freelancer_name}</h3>
            <div className="flex items-center gap-2">
              {[1, 2, 3, 4, 5].map((star) => (
                <button
                  key={star}
                  type="button"
                  onClick={() => setRating(star)}
                  className="p-1.5"
                  aria-label={`Rate ${star} stars`}
                >
                  <Star className={`w-6 h-6 ${star <= rating ? 'text-amber-400 fill-amber-400' : 'text-slate-600'}`} />
                </button>
              ))}
            </div>
            <textarea
              rows={3}
              required
              value={comment}
              onChange={(e) => setComment(e.target.value)}
              placeholder="Share feedback on communication, craft quality, and delivery..."
              className="glass-input w-full rounded-xl px-3.5 py-2.5 text-sm"
            />
            <div className="flex justify-end gap-3">
              <button type="button" onClick={() => setReviewTarget(null)} className="px-4 py-2 text-xs text-slate-300">
                Cancel
              </button>
              <button type="submit" className="gold-cta px-5 py-2.5 text-xs rounded-xl">
                Submit Review
              </button>
            </div>
          </form>
        </div>
      )}
    </div>
  );
};
