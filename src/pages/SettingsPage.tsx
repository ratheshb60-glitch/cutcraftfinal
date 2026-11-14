import React, { useEffect, useState } from 'react';
import { LogOut, Upload } from 'lucide-react';
import { api, uploadOriginalFile } from '../api/client';
import type { User } from '../types';

interface SettingsPageProps {
  user: User;
  onUserUpdated: (u: User) => void;
  onLogout: () => void;
}

export const SettingsPage: React.FC<SettingsPageProps> = ({ user, onUserUpdated, onLogout }) => {
  const [fullName, setFullName] = useState(user.full_name);
  const [profileMsg, setProfileMsg] = useState('');

  const [currentPassword, setCurrentPassword] = useState('');
  const [newPassword, setNewPassword] = useState('');
  const [pwMsg, setPwMsg] = useState('');
  const [pwErr, setPwErr] = useState('');

  const [commissionRate, setCommissionRate] = useState('10');
  const [commMsg, setCommMsg] = useState('');

  useEffect(() => {
    api.previewCommission(1000).then((res) => setCommissionRate(String(res.commission_rate))).catch(() => {});
  }, []);

  const handleProfileSave = async (e: React.FormEvent) => {
    e.preventDefault();
    setProfileMsg('');
    try {
      const res = await api.updateUserProfile({ full_name: fullName.trim() });
      onUserUpdated(res.user);
      setProfileMsg('Account details updated.');
    } catch (err) {
      setProfileMsg(err instanceof Error ? err.message : 'Update failed.');
    }
  };

  const handleAvatarChange = async (file: File) => {
    try {
      const up = await uploadOriginalFile(file);
      const res = await api.updateUserProfile({ avatar_url: up.file_path });
      onUserUpdated(res.user);
      setProfileMsg('Profile photo updated.');
    } catch (err) {
      setProfileMsg(err instanceof Error ? err.message : 'Upload failed.');
    }
  };

  const handlePasswordChange = async (e: React.FormEvent) => {
    e.preventDefault();
    setPwErr('');
    setPwMsg('');
    try {
      const res = await api.changePassword(currentPassword, newPassword);
      setPwMsg(res.message);
      setCurrentPassword('');
      setNewPassword('');
    } catch (err) {
      setPwErr(err instanceof Error ? err.message : 'Password update failed.');
    }
  };

  const handleCommissionSave = async (e: React.FormEvent) => {
    e.preventDefault();
    setCommMsg('');
    try {
      const res = await api.updateCommissionRate(Number(commissionRate));
      setCommMsg(`${res.message} Active rate: ${res.commission_rate}%`);
    } catch (err) {
      setCommMsg(err instanceof Error ? err.message : 'Could not update rate.');
    }
  };

  return (
    <div className="max-w-4xl mx-auto px-4 sm:px-6 py-10 space-y-8">
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <p className="text-xs text-amber-400 font-medium">CUTCRAFT Account</p>
          <h1 className="font-display text-3xl font-bold text-white mt-1">Settings &amp; Security</h1>
        </div>
        <button
          type="button"
          onClick={onLogout}
          className="px-4 py-2.5 text-xs font-medium rounded-xl border border-rose-400/35 text-rose-200 hover:bg-rose-500/10 inline-flex items-center gap-2 self-start sm:self-auto min-h-[44px]"
        >
          <LogOut className="w-4 h-4" />
          <span>Sign Out</span>
        </button>
      </div>

      {/* Edit Profile */}
      <form onSubmit={handleProfileSave} className="glass-panel rounded-3xl p-6 space-y-5">
        <div className="flex flex-col sm:flex-row sm:items-center gap-4">
          <img
            src={user.avatar_url || '/assets/generated/creator_portrait_designer.jpg'}
            alt={user.full_name}
            referrerPolicy="no-referrer"
            className="w-16 h-16 rounded-2xl object-cover border border-white/20"
          />
          <div>
            <p className="text-sm font-semibold text-white">{user.email}</p>
            <p className="text-xs text-slate-400 capitalize">Account Type: {user.account_type}</p>
            <label className="inline-flex items-center gap-1.5 text-xs text-amber-400 hover:text-amber-300 cursor-pointer mt-1">
              <Upload className="w-3.5 h-3.5" />
              <span>Change Profile Photo</span>
              <input
                type="file"
                accept=".jpg,.jpeg,.png,.webp"
                className="hidden"
                onChange={(e) => {
                  const f = e.target.files?.[0];
                  if (f) handleAvatarChange(f);
                }}
              />
            </label>
          </div>
        </div>

        <div>
          <label className="block text-xs text-slate-300 mb-1">Full Name</label>
          <input
            type="text"
            required
            value={fullName}
            onChange={(e) => setFullName(e.target.value)}
            className="glass-input w-full rounded-xl px-4 py-2.5 text-sm"
          />
        </div>

        {profileMsg && <p className="text-xs text-amber-300">{profileMsg}</p>}
        <button type="submit" className="gold-cta px-5 py-2.5 text-xs rounded-xl min-h-[44px]">
          Save Profile Changes
        </button>
      </form>

      {/* Password Management */}
      <form onSubmit={handlePasswordChange} className="glass-panel rounded-3xl p-6 space-y-4">
        <h2 className="font-display text-xl font-bold text-white">Password Management</h2>
        <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
          <div>
            <label className="block text-xs text-slate-300 mb-1">Current Password</label>
            <input
              type="password"
              required
              value={currentPassword}
              onChange={(e) => setCurrentPassword(e.target.value)}
              className="glass-input w-full rounded-xl px-4 py-2.5 text-sm"
            />
          </div>
          <div>
            <label className="block text-xs text-slate-300 mb-1">New Password</label>
            <input
              type="password"
              required
              minLength={6}
              value={newPassword}
              onChange={(e) => setNewPassword(e.target.value)}
              className="glass-input w-full rounded-xl px-4 py-2.5 text-sm"
            />
          </div>
        </div>
        {pwErr && <p className="text-xs text-rose-300">{pwErr}</p>}
        {pwMsg && <p className="text-xs text-emerald-300">{pwMsg}</p>}
        <button type="submit" className="px-5 py-2.5 text-xs font-medium rounded-xl border border-white/15 bg-white/[0.05] text-white hover:border-amber-400/50 min-h-[44px]">
          Update Password
        </button>
      </form>

      {/* Configurable Platform Commission Setting */}
      <form onSubmit={handleCommissionSave} className="glass-panel rounded-3xl p-6 space-y-4">
        <div>
          <h2 className="font-display text-xl font-bold text-white">Platform Commission Configuration</h2>
          <p className="text-xs text-slate-400 mt-1">
            Adjust the CUTCRAFT platform commission percentage stored in SQLite (used by commission.c for all new project requests).
          </p>
        </div>
        <div className="flex flex-col sm:flex-row gap-3 sm:items-end">
          <div className="flex-1 max-w-xs">
            <label className="block text-xs text-slate-300 mb-1">Commission Rate (%)</label>
            <input
              type="number"
              step="0.5"
              min="0"
              max="50"
              value={commissionRate}
              onChange={(e) => setCommissionRate(e.target.value)}
              className="glass-input w-full rounded-xl px-4 py-2.5 text-sm font-mono-num"
            />
          </div>
          <button type="submit" className="px-5 py-2.5 text-xs font-medium rounded-xl border border-amber-400/40 text-amber-300 hover:bg-amber-500/10 min-h-[44px]">
            Apply Rate
          </button>
        </div>
        {commMsg && <p className="text-xs text-amber-300">{commMsg}</p>}
      </form>
    </div>
  );
};
