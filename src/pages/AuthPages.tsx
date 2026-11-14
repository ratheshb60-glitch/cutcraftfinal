import React, { useState } from 'react';
import { Eye, EyeOff } from 'lucide-react';
import { api, setStoredToken } from '../api/client';
import type { PageRoute, User } from '../types';
import { CUTCRAFT_LOGO_SRC } from '../components/Navbar';

interface AuthPagesProps {
  mode: 'login' | 'register';
  onNavigate: (page: PageRoute) => void;
  onAuthenticated: (user: User) => void;
}

const PROFESSIONS = [
  'Video Editor',
  'Photo Editor',
  'Photographer',
  'Videographer',
  'Graphic Designer',
  'Motion Graphics Designer',
  'Thumbnail Designer',
  'Creative Specialist',
];

export const AuthPages: React.FC<AuthPagesProps> = ({ mode, onNavigate, onAuthenticated }) => {
  const [fullName, setFullName] = useState('');
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [confirmPassword, setConfirmPassword] = useState('');
  const [accountType, setAccountType] = useState<'client' | 'freelancer'>('client');
  const [profession, setProfession] = useState('Video Editor');
  const [showPw, setShowPw] = useState(false);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');
  const [forgotOpen, setForgotOpen] = useState(false);
  const [forgotEmail, setForgotEmail] = useState('');
  const [forgotMsg, setForgotMsg] = useState('');

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');

    if (mode === 'register') {
      if (fullName.trim().length < 2) {
        setError('Please enter your full name.');
        return;
      }
      if (password.length < 6) {
        setError('Password must be at least 6 characters.');
        return;
      }
      if (password !== confirmPassword) {
        setError('Passwords do not match.');
        return;
      }
    }

    setLoading(true);
    try {
      const res =
        mode === 'register'
          ? await api.register({
              full_name: fullName.trim(),
              email: email.trim(),
              password,
              account_type: accountType,
              profession: accountType === 'freelancer' ? profession : undefined,
            })
          : await api.login({ email: email.trim(), password });

      setStoredToken(res.token);
      onAuthenticated(res.user);
      onNavigate('dashboard');
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Authentication failed.');
    } finally {
      setLoading(false);
    }
  };

  const handleForgot = async (e: React.FormEvent) => {
    e.preventDefault();
    setForgotMsg('');
    try {
      const res = await api.forgotPassword(forgotEmail.trim());
      setForgotMsg(res.message);
    } catch (err) {
      setForgotMsg(err instanceof Error ? err.message : 'Could not process reset.');
    }
  };

  const fillDemo = (demoEmail: string) => {
    setEmail(demoEmail);
    setPassword('Cutcraft@2026');
    setError('');
  };

  return (
    <div className="max-w-lg mx-auto px-4 py-12 md:py-16">
      <div className="glass-panel rounded-3xl p-6 sm:p-8 space-y-6">
        <div className="flex items-center gap-3">
          <img
            src={CUTCRAFT_LOGO_SRC}
            alt="CUTCRAFT logo"
            referrerPolicy="no-referrer"
            className="w-11 h-11 rounded-full object-cover border border-amber-400/50"
          />
          <div>
            <p className="text-xs font-medium text-amber-400">CUTCRAFT</p>
            <h1 className="font-display text-2xl font-bold text-white">
              {mode === 'register' ? 'Create Your Account' : 'Sign In to CUTCRAFT'}
            </h1>
          </div>
        </div>

        {error && (
          <div className="p-3.5 rounded-xl bg-rose-500/10 border border-rose-400/30 text-sm text-rose-200" role="alert">
            {error}
          </div>
        )}

        <form onSubmit={handleSubmit} className="space-y-4">
          {mode === 'register' && (
            <>
              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1.5">Account Type</label>
                <div className="grid grid-cols-2 gap-2 p-1 rounded-xl bg-black/30 border border-white/10">
                  <button
                    type="button"
                    onClick={() => setAccountType('client')}
                    className={`py-2.5 text-xs font-semibold rounded-lg transition-colors min-h-[40px] ${
                      accountType === 'client' ? 'bg-amber-500 text-slate-950' : 'text-slate-300 hover:text-white'
                    }`}
                  >
                    Client · Hire Talent
                  </button>
                  <button
                    type="button"
                    onClick={() => setAccountType('freelancer')}
                    className={`py-2.5 text-xs font-semibold rounded-lg transition-colors min-h-[40px] ${
                      accountType === 'freelancer' ? 'bg-amber-500 text-slate-950' : 'text-slate-300 hover:text-white'
                    }`}
                  >
                    Freelancer · Offer Work
                  </button>
                </div>
              </div>

              <div>
                <label htmlFor="cc-fullname" className="block text-xs font-medium text-slate-300 mb-1.5">
                  Full Name
                </label>
                <input
                  id="cc-fullname"
                  type="text"
                  required
                  value={fullName}
                  onChange={(e) => setFullName(e.target.value)}
                  placeholder="Enter your full name"
                  className="glass-input w-full rounded-xl px-4 py-3 text-sm"
                />
              </div>

              {accountType === 'freelancer' && (
                <div>
                  <label htmlFor="cc-profession" className="block text-xs font-medium text-slate-300 mb-1.5">
                    Primary Creative Profession
                  </label>
                  <select
                    id="cc-profession"
                    value={profession}
                    onChange={(e) => setProfession(e.target.value)}
                    className="glass-input w-full rounded-xl px-4 py-3 text-sm"
                  >
                    {PROFESSIONS.map((p) => (
                      <option key={p} value={p} className="bg-slate-900">
                        {p}
                      </option>
                    ))}
                  </select>
                </div>
              )}
            </>
          )}

          <div>
            <label htmlFor="cc-email" className="block text-xs font-medium text-slate-300 mb-1.5">
              Email Address
            </label>
            <input
              id="cc-email"
              type="email"
              required
              value={email}
              onChange={(e) => setEmail(e.target.value)}
              placeholder="name@example.com"
              className="glass-input w-full rounded-xl px-4 py-3 text-sm"
            />
          </div>

          <div>
            <div className="flex items-center justify-between mb-1.5">
              <label htmlFor="cc-password" className="text-xs font-medium text-slate-300">
                Password
              </label>
              {mode === 'login' && (
                <button
                  type="button"
                  onClick={() => setForgotOpen(!forgotOpen)}
                  className="text-xs text-amber-400 hover:text-amber-300"
                >
                  Forgot password?
                </button>
              )}
            </div>
            <div className="relative">
              <input
                id="cc-password"
                type={showPw ? 'text' : 'password'}
                required
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                placeholder="Minimum 6 characters"
                className="glass-input w-full rounded-xl px-4 py-3 pr-12 text-sm"
              />
              <button
                type="button"
                onClick={() => setShowPw(!showPw)}
                aria-label={showPw ? 'Hide password' : 'Show password'}
                className="absolute right-2 top-1/2 -translate-y-1/2 min-w-[40px] min-h-[40px] flex items-center justify-center text-slate-400 hover:text-white"
              >
                {showPw ? <EyeOff className="w-4 h-4" /> : <Eye className="w-4 h-4" />}
              </button>
            </div>
          </div>

          {mode === 'register' && (
            <div>
              <label htmlFor="cc-confirm" className="block text-xs font-medium text-slate-300 mb-1.5">
                Confirm Password
              </label>
              <input
                id="cc-confirm"
                type={showPw ? 'text' : 'password'}
                required
                value={confirmPassword}
                onChange={(e) => setConfirmPassword(e.target.value)}
                placeholder="Re-enter password"
                className="glass-input w-full rounded-xl px-4 py-3 text-sm"
              />
            </div>
          )}

          <button
            type="submit"
            disabled={loading}
            className="gold-cta w-full py-3.5 text-sm rounded-xl min-h-[44px] disabled:opacity-60"
          >
            {loading ? 'Authenticating...' : mode === 'register' ? 'Create CUTCRAFT Account' : 'Sign In'}
          </button>
        </form>

        {mode === 'login' && (
          <div className="pt-4 border-t border-white/[0.08] space-y-2">
            <p className="text-xs text-slate-400">Sample Seeded Accounts (for quick testing):</p>
            <div className="flex flex-wrap gap-2">
              <button
                type="button"
                onClick={() => fillDemo('client@cutcraft.studio')}
                className="px-3 py-2 text-xs rounded-lg border border-white/10 bg-white/[0.04] text-slate-300 hover:text-white hover:border-amber-400/40"
              >
                Use Sample Client
              </button>
              <button
                type="button"
                onClick={() => fillDemo('elena@cutcraft.studio')}
                className="px-3 py-2 text-xs rounded-lg border border-white/10 bg-white/[0.04] text-slate-300 hover:text-white hover:border-amber-400/40"
              >
                Use Sample Freelancer
              </button>
            </div>
          </div>
        )}

        {mode === 'login' && forgotOpen && (
          <form onSubmit={handleForgot} className="pt-4 border-t border-white/[0.08] space-y-3">
            <p className="text-xs font-medium text-slate-200">Reset Account Password</p>
            <div className="flex gap-2">
              <input
                type="email"
                required
                value={forgotEmail}
                onChange={(e) => setForgotEmail(e.target.value)}
                placeholder="Enter your registered email"
                className="glass-input flex-1 rounded-xl px-3 py-2 text-xs"
              />
              <button
                type="submit"
                className="px-4 py-2 text-xs font-medium rounded-xl bg-white/10 hover:bg-white/15 text-white"
              >
                Send
              </button>
            </div>
            {forgotMsg && <p className="text-xs text-amber-300">{forgotMsg}</p>}
          </form>
        )}

        <p className="text-center text-xs text-slate-400 pt-2">
          {mode === 'register' ? (
            <>
              Already have a CUTCRAFT account?{' '}
              <button type="button" onClick={() => onNavigate('login')} className="text-amber-400 hover:underline">
                Sign In
              </button>
            </>
          ) : (
            <>
              New to CUTCRAFT?{' '}
              <button type="button" onClick={() => onNavigate('register')} className="text-amber-400 hover:underline">
                Create an account
              </button>
            </>
          )}
        </p>
      </div>
    </div>
  );
};
