import React, { useEffect, useState } from 'react';
import { Check, Pencil, Play, Search, Trash2, Upload, X } from 'lucide-react';
import { api, uploadOriginalFile } from '../api/client';
import type {
  EarningsSummary,
  Freelancer,
  PageRoute,
  PortfolioItem,
  Project,
  ProjectRequest,
  User,
} from '../types';
import { FreelancerCard } from '../components/FreelancerCard';

interface DashboardPageProps {
  user: User;
  onUserUpdated: (u: User) => void;
  onNavigate: (page: PageRoute) => void;
  onSelectCategory: (cat: string) => void;
  onSelectFreelancer: (id: number) => void;
  onOpenPortfolio: (item: PortfolioItem) => void;
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

const PORTFOLIO_CATEGORIES = [
  'Video Editors',
  'Photo Editors',
  'Photographers',
  'Videographers',
  'Graphic Designers',
  'Motion Graphics Designers',
  'Thumbnail Designers',
];

export const DashboardPage: React.FC<DashboardPageProps> = ({
  user,
  onUserUpdated,
  onNavigate,
  onSelectCategory,
  onSelectFreelancer,
  onOpenPortfolio,
}) => {
  const [featured, setFeatured] = useState<Freelancer[]>([]);
  const [trending, setTrending] = useState<PortfolioItem[]>([]);
  const [requests, setRequests] = useState<ProjectRequest[]>([]);
  const [projects, setProjects] = useState<Project[]>([]);
  const [earnings, setEarnings] = useState<EarningsSummary | null>(null);
  const [searchInput, setSearchInput] = useState('');

  /* Freelancer Profile & Portfolio states */
  const [profession, setProfession] = useState('Video Editor');
  const [bio, setBio] = useState('');
  const [skills, setSkills] = useState('');
  const [startingPrice, setStartingPrice] = useState('2500');
  const [location, setLocation] = useState('Remote');
  const [myPortfolio, setMyPortfolio] = useState<PortfolioItem[]>([]);
  const [profileMsg, setProfileMsg] = useState('');

  /* Portfolio upload states */
  const [itemTitle, setItemTitle] = useState('');
  const [itemDesc, setItemDesc] = useState('');
  const [itemCat, setItemCat] = useState('Video Editors');
  const [mediaFile, setMediaFile] = useState<File | null>(null);
  const [thumbFile, setThumbFile] = useState<File | null>(null);
  const [uploadProgress, setUploadProgress] = useState(0);
  const [uploading, setUploading] = useState(false);
  const [uploadMsg, setUploadMsg] = useState('');
  const [uploadErr, setUploadErr] = useState('');

  /* Edit portfolio inline */
  const [editingId, setEditingId] = useState<number | null>(null);
  const [editTitle, setEditTitle] = useState('');
  const [editDesc, setEditDesc] = useState('');

  const loadData = async () => {
    try {
      const [reqRes, projRes, earnRes] = await Promise.all([
        api.getProjectRequests(),
        api.getProjects(),
        api.getEarnings(),
      ]);
      setRequests(reqRes.requests);
      setProjects(projRes.projects);
      setEarnings(earnRes);

      if (user.account_type === 'client') {
        const [fRes, pRes] = await Promise.all([api.getFeaturedFreelancers(), api.getTrendingPortfolios()]);
        setFeatured(fRes.freelancers);
        setTrending(pRes.portfolios);
      } else if (user.freelancer_profile_id > 0) {
        const detail = await api.getFreelancerDetail(user.freelancer_profile_id);
        setProfession(detail.freelancer.profession);
        setBio(detail.freelancer.bio);
        setSkills(detail.freelancer.skills);
        setStartingPrice(String(detail.freelancer.starting_price));
        setLocation(detail.freelancer.location);
        setMyPortfolio(detail.portfolio);
      }
    } catch {
      /* handled softly */
    }
  };

  useEffect(() => {
    loadData();
  }, [user.id, user.freelancer_profile_id, user.account_type]);

  if (user.account_type === 'client') {
    return (
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-10 space-y-12">
        {/* Client Header */}
        <div className="glass-panel rounded-3xl p-6 sm:p-8 flex flex-col lg:flex-row lg:items-center justify-between gap-6">
          <div>
            <p className="text-xs text-amber-400 font-medium">CUTCRAFT Client Studio</p>
            <h1 className="font-display text-2xl sm:text-3xl font-bold text-white mt-1">
              Welcome back, {user.full_name}
            </h1>
            <p className="text-sm text-slate-400 mt-1">
              Find creative specialists, manage project requests, and track active deliverables.
            </p>
          </div>
          <form
            onSubmit={(e) => {
              e.preventDefault();
              onSelectCategory(searchInput.trim() || 'All');
            }}
            className="flex flex-col sm:flex-row gap-3 w-full lg:w-auto"
          >
            <div className="relative flex-1 sm:w-72">
              <Search className="w-4 h-4 text-slate-400 absolute left-3.5 top-1/2 -translate-y-1/2" />
              <input
                type="text"
                value={searchInput}
                onChange={(e) => setSearchInput(e.target.value)}
                placeholder="Find video editors, photographers..."
                className="glass-input w-full rounded-xl pl-10 pr-4 py-2.5 text-sm"
              />
            </div>
            <button type="submit" className="gold-cta px-5 py-2.5 text-xs rounded-xl whitespace-nowrap min-h-[44px]">
              Search Creators
            </button>
          </form>
        </div>

        {/* Client Overview Metrics */}
        <div className="grid grid-cols-2 lg:grid-cols-4 gap-4">
          <div className="glass-panel rounded-2xl p-5">
            <p className="text-xs text-slate-400">Active Projects</p>
            <p className="font-mono-num text-2xl font-bold text-white mt-1">
              {projects.filter((p) => p.status !== 'completed' && p.status !== 'cancelled').length}
            </p>
          </div>
          <div className="glass-panel rounded-2xl p-5">
            <p className="text-xs text-slate-400">Pending Requests</p>
            <p className="font-mono-num text-2xl font-bold text-amber-400 mt-1">
              {requests.filter((r) => r.status === 'pending').length}
            </p>
          </div>
          <div className="glass-panel rounded-2xl p-5">
            <p className="text-xs text-slate-400">Completed Projects</p>
            <p className="font-mono-num text-2xl font-bold text-white mt-1">
              {projects.filter((p) => p.status === 'completed').length}
            </p>
          </div>
          <div className="glass-panel rounded-2xl p-5">
            <p className="text-xs text-slate-400">Simulated Spend</p>
            <p className="font-mono-num text-2xl font-bold text-white mt-1">
              ₹{(earnings?.total_gross || 0).toLocaleString('en-IN')}
            </p>
          </div>
        </div>

        {/* Recommended / Featured Professionals */}
        <section className="space-y-4">
          <div className="flex items-center justify-between">
            <h2 className="font-display text-2xl font-bold text-white">Recommended Professionals</h2>
            <button type="button" onClick={() => onNavigate('discover')} className="text-sm text-amber-400 hover:underline">
              Browse all
            </button>
          </div>
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-5">
            {featured.map((f) => (
              <FreelancerCard key={f.id} freelancer={f} onSelect={onSelectFreelancer} />
            ))}
          </div>
        </section>

        {/* Popular Categories */}
        <section className="space-y-4">
          <h2 className="font-display text-2xl font-bold text-white">Popular Categories</h2>
          <div className="grid grid-cols-2 sm:grid-cols-4 gap-3">
            {PORTFOLIO_CATEGORIES.map((c) => (
              <button
                key={c}
                type="button"
                onClick={() => onSelectCategory(c)}
                className="glass-panel glass-panel-hover rounded-xl px-4 py-3.5 text-left text-sm font-medium text-slate-200 hover:text-amber-300"
              >
                {c}
              </button>
            ))}
          </div>
        </section>

        {/* Trending Portfolios */}
        <section className="space-y-4">
          <h2 className="font-display text-2xl font-bold text-white">Trending Portfolios</h2>
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-5">
            {trending.slice(0, 6).map((item) => (
              <button
                key={item.id}
                type="button"
                onClick={() => onOpenPortfolio(item)}
                className="glass-panel glass-panel-hover rounded-2xl overflow-hidden text-left"
              >
                <div className="relative h-48 bg-slate-900">
                  <img
                    src={item.thumbnail_path || item.file_path}
                    alt={item.title}
                    referrerPolicy="no-referrer"
                    className="w-full h-full object-cover"
                  />
                  <div className="absolute inset-0 bg-gradient-to-t from-[#050814]/85 via-transparent to-transparent" />
                  <div className="absolute bottom-3 left-4 right-4">
                    <p className="text-xs text-amber-300">{item.creator_name} · {item.category}</p>
                    <p className="font-display text-base font-semibold text-white truncate">{item.title}</p>
                  </div>
                </div>
              </button>
            ))}
          </div>
        </section>
      </div>
    );
  }

  /* FREELANCER DASHBOARD */
  const handleSaveProfile = async (e: React.FormEvent) => {
    e.preventDefault();
    setProfileMsg('');
    try {
      await api.upsertFreelancerProfile({
        profession,
        bio,
        skills,
        starting_price: Number(startingPrice) || 1500,
        location,
      });
      setProfileMsg('Creator profile saved to SQLite database.');
    } catch (err) {
      setProfileMsg(err instanceof Error ? err.message : 'Failed to save profile.');
    }
  };

  const handleAvatarUpload = async (file: File) => {
    try {
      const uploaded = await uploadOriginalFile(file);
      const res = await api.updateUserProfile({ avatar_url: uploaded.file_path });
      onUserUpdated(res.user);
      setProfileMsg('Profile photo updated.');
    } catch (err) {
      setProfileMsg(err instanceof Error ? err.message : 'Avatar upload failed.');
    }
  };

  const handlePortfolioUpload = async (e: React.FormEvent) => {
    e.preventDefault();
    setUploadErr('');
    setUploadMsg('');
    if (!mediaFile || !itemTitle.trim()) {
      setUploadErr('Please select an image or video file and enter a title.');
      return;
    }

    setUploading(true);
    setUploadProgress(0);
    try {
      const uploadedMedia = await uploadOriginalFile(mediaFile, (pct) => setUploadProgress(pct));
      if (uploadedMedia.media_type === 'document') {
        throw new Error('Portfolio items must be images or videos.');
      }

      let thumbPath = uploadedMedia.file_path;
      if (thumbFile) {
        const upThumb = await uploadOriginalFile(thumbFile);
        thumbPath = upThumb.file_path;
      } else if (uploadedMedia.media_type === 'video') {
        thumbPath = '/assets/generated/portfolio_cinema_edit.jpg';
      }

      await api.addPortfolioItem({
        title: itemTitle.trim(),
        description: itemDesc.trim(),
        category: itemCat,
        media_type: uploadedMedia.media_type,
        file_path: uploadedMedia.file_path,
        thumbnail_path: thumbPath,
        original_filename: uploadedMedia.original_filename,
        file_size_bytes: uploadedMedia.file_size_bytes,
        mime_type: uploadedMedia.mime_type,
      });

      setItemTitle('');
      setItemDesc('');
      setMediaFile(null);
      setThumbFile(null);
      setUploadProgress(100);
      setUploadMsg(`Uploaded original ${uploadedMedia.media_type} (${uploadedMedia.original_filename}) without compression.`);
      await loadData();
    } catch (err) {
      setUploadErr(err instanceof Error ? err.message : 'Upload failed.');
    } finally {
      setUploading(false);
    }
  };

  const handleRespondRequest = async (reqId: number, action: 'accept' | 'reject') => {
    try {
      await api.respondProjectRequest(reqId, action);
      await loadData();
    } catch {
      /* ignore */
    }
  };

  const handleSavePortfolioEdit = async (id: number) => {
    if (!editTitle.trim()) return;
    await api.updatePortfolioItem(id, { title: editTitle.trim(), description: editDesc.trim() });
    setEditingId(null);
    await loadData();
  };

  const handleDeletePortfolio = async (id: number) => {
    await api.deletePortfolioItem(id);
    await loadData();
  };

  const pendingRequests = requests.filter((r) => r.status === 'pending');

  return (
    <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-10 space-y-10">
      {/* Freelancer Header */}
      <div className="glass-panel rounded-3xl p-6 sm:p-8 flex flex-col md:flex-row md:items-center justify-between gap-6">
        <div className="flex items-center gap-4">
          <img
            src={user.avatar_url || '/assets/generated/creator_portrait_designer.jpg'}
            alt={user.full_name}
            referrerPolicy="no-referrer"
            className="w-16 h-16 rounded-2xl object-cover border border-white/20"
          />
          <div>
            <p className="text-xs text-amber-400 font-medium">CUTCRAFT Creator Studio</p>
            <h1 className="font-display text-2xl sm:text-3xl font-bold text-white">{user.full_name}</h1>
            <p className="text-xs text-slate-400 mt-0.5">{profession} · {location}</p>
          </div>
        </div>

        <div className="flex flex-wrap gap-3">
          <button
            type="button"
            onClick={() => onSelectFreelancer(user.freelancer_profile_id)}
            className="px-4 py-2.5 text-xs font-medium rounded-xl border border-white/15 bg-white/[0.04] text-white hover:border-amber-400/50 min-h-[44px]"
          >
            View Public Profile
          </button>
          <button
            type="button"
            onClick={() => onNavigate('projects')}
            className="gold-cta px-4 py-2.5 text-xs rounded-xl min-h-[44px]"
          >
            Manage Active Projects
          </button>
        </div>
      </div>

      {/* Earnings Summary */}
      <div className="grid grid-cols-2 lg:grid-cols-4 gap-4">
        <div className="glass-panel rounded-2xl p-5">
          <p className="text-xs text-slate-400">Net Earnings (Simulated)</p>
          <p className="font-mono-num text-2xl font-bold text-amber-400 mt-1">
            ₹{(earnings?.total_net_earnings || 0).toLocaleString('en-IN')}
          </p>
        </div>
        <div className="glass-panel rounded-2xl p-5">
          <p className="text-xs text-slate-400">Gross Project Volume</p>
          <p className="font-mono-num text-2xl font-bold text-white mt-1">
            ₹{(earnings?.total_gross || 0).toLocaleString('en-IN')}
          </p>
        </div>
        <div className="glass-panel rounded-2xl p-5">
          <p className="text-xs text-slate-400">Platform Commission ({earnings?.commission_rate ?? 10}%)</p>
          <p className="font-mono-num text-2xl font-bold text-slate-300 mt-1">
            ₹{(earnings?.total_commission || 0).toLocaleString('en-IN')}
          </p>
        </div>
        <div className="glass-panel rounded-2xl p-5">
          <p className="text-xs text-slate-400">Incoming Requests</p>
          <p className="font-mono-num text-2xl font-bold text-white mt-1">{pendingRequests.length}</p>
        </div>
      </div>

      {/* Incoming Project Requests */}
      <section className="glass-panel rounded-2xl p-6 space-y-4">
        <h2 className="font-display text-xl font-bold text-white">Incoming Project Requests</h2>
        {pendingRequests.length === 0 ? (
          <p className="text-sm text-slate-400">No pending project requests right now.</p>
        ) : (
          <div className="divide-y divide-white/[0.08]">
            {pendingRequests.map((req) => (
              <div key={req.id} className="py-4 flex flex-col md:flex-row md:items-center justify-between gap-4">
                <div className="space-y-1">
                  <h3 className="font-display text-base font-semibold text-white">{req.title}</h3>
                  <p className="text-xs text-slate-400">
                    Client: {req.client_name} · Deadline: {req.deadline} · Budget:{' '}
                    <span className="font-mono-num text-white">₹{req.budget.toLocaleString('en-IN')}</span> · Your payout:{' '}
                    <span className="font-mono-num text-amber-300">₹{req.freelancer_earnings.toLocaleString('en-IN')}</span>
                  </p>
                  <p className="text-sm text-slate-300">{req.description}</p>
                </div>
                <div className="flex items-center gap-2 shrink-0">
                  <button
                    type="button"
                    onClick={() => handleRespondRequest(req.id, 'accept')}
                    className="gold-cta px-4 py-2 text-xs rounded-lg inline-flex items-center gap-1.5 min-h-[40px]"
                  >
                    <Check className="w-4 h-4" />
                    <span>Accept</span>
                  </button>
                  <button
                    type="button"
                    onClick={() => handleRespondRequest(req.id, 'reject')}
                    className="px-4 py-2 text-xs font-medium rounded-lg border border-rose-400/30 text-rose-200 hover:bg-rose-500/10 inline-flex items-center gap-1.5 min-h-[40px]"
                  >
                    <X className="w-4 h-4" />
                    <span>Reject</span>
                  </button>
                </div>
              </div>
            ))}
          </div>
        )}
      </section>

      {/* Profile Management & Portfolio Upload */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <form onSubmit={handleSaveProfile} className="glass-panel rounded-2xl p-6 space-y-4">
          <div className="flex items-center justify-between">
            <h2 className="font-display text-xl font-bold text-white">Creator Profile Settings</h2>
            <label className="text-xs text-amber-400 hover:text-amber-300 cursor-pointer">
              Upload Photo
              <input
                type="file"
                accept="image/jpeg,image/png,image/webp"
                className="hidden"
                onChange={(e) => {
                  const f = e.target.files?.[0];
                  if (f) handleAvatarUpload(f);
                }}
              />
            </label>
          </div>

          <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
            <div>
              <label className="block text-xs text-slate-300 mb-1">Profession</label>
              <select
                value={profession}
                onChange={(e) => setProfession(e.target.value)}
                className="glass-input w-full rounded-xl px-3.5 py-2.5 text-sm"
              >
                {PROFESSIONS.map((p) => (
                  <option key={p} value={p} className="bg-slate-900">{p}</option>
                ))}
              </select>
            </div>
            <div>
              <label className="block text-xs text-slate-300 mb-1">Starting Price (₹)</label>
              <input
                type="number"
                min="100"
                value={startingPrice}
                onChange={(e) => setStartingPrice(e.target.value)}
                className="glass-input w-full rounded-xl px-3.5 py-2.5 text-sm font-mono-num"
              />
            </div>
          </div>

          <div>
            <label className="block text-xs text-slate-300 mb-1">Location</label>
            <input
              type="text"
              value={location}
              onChange={(e) => setLocation(e.target.value)}
              className="glass-input w-full rounded-xl px-3.5 py-2.5 text-sm"
            />
          </div>

          <div>
            <label className="block text-xs text-slate-300 mb-1">Skills (comma separated)</label>
            <input
              type="text"
              value={skills}
              onChange={(e) => setSkills(e.target.value)}
              placeholder="DaVinci Resolve, Premiere Pro, Sound Design"
              className="glass-input w-full rounded-xl px-3.5 py-2.5 text-sm"
            />
          </div>

          <div>
            <label className="block text-xs text-slate-300 mb-1">Bio</label>
            <textarea
              rows={3}
              value={bio}
              onChange={(e) => setBio(e.target.value)}
              className="glass-input w-full rounded-xl px-3.5 py-2.5 text-sm"
            />
          </div>

          {profileMsg && <p className="text-xs text-amber-300">{profileMsg}</p>}
          <button type="submit" className="gold-cta px-5 py-2.5 text-xs rounded-xl min-h-[44px]">
            Save Creator Profile
          </button>
        </form>

        {/* Upload Portfolio Photo or Video */}
        <form onSubmit={handlePortfolioUpload} className="glass-panel rounded-2xl p-6 space-y-4">
          <div>
            <h2 className="font-display text-xl font-bold text-white">Upload Portfolio Photo or Video</h2>
            <p className="text-xs text-slate-400 mt-0.5">
              Original uploaded files are preserved uncompressed on disk (Cloudflare R2 &amp; Mux adaptive pipeline ready).
            </p>
          </div>

          <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
            <div>
              <label className="block text-xs text-slate-300 mb-1">Project Title</label>
              <input
                type="text"
                required
                value={itemTitle}
                onChange={(e) => setItemTitle(e.target.value)}
                placeholder="e.g. Brand Commercial Reel"
                className="glass-input w-full rounded-xl px-3.5 py-2.5 text-sm"
              />
            </div>
            <div>
              <label className="block text-xs text-slate-300 mb-1">Category</label>
              <select
                value={itemCat}
                onChange={(e) => setItemCat(e.target.value)}
                className="glass-input w-full rounded-xl px-3.5 py-2.5 text-sm"
              >
                {PORTFOLIO_CATEGORIES.map((c) => (
                  <option key={c} value={c} className="bg-slate-900">{c}</option>
                ))}
              </select>
            </div>
          </div>

          <div>
            <label className="block text-xs text-slate-300 mb-1">Description</label>
            <textarea
              rows={2}
              value={itemDesc}
              onChange={(e) => setItemDesc(e.target.value)}
              placeholder="Camera, software, or creative notes..."
              className="glass-input w-full rounded-xl px-3.5 py-2.5 text-sm"
            />
          </div>

          <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
            <label className="glass-input rounded-xl px-4 py-3 text-xs text-slate-300 cursor-pointer flex items-center gap-2">
              <Upload className="w-4 h-4 text-amber-400 shrink-0" />
              <span className="truncate">{mediaFile ? mediaFile.name : 'Select Photo or Video (JPG/PNG/MP4/MOV)'}</span>
              <input
                type="file"
                accept=".jpg,.jpeg,.png,.webp,.mp4,.webm,.mov"
                className="hidden"
                onChange={(e) => setMediaFile(e.target.files?.[0] || null)}
              />
            </label>

            <label className="glass-input rounded-xl px-4 py-3 text-xs text-slate-300 cursor-pointer flex items-center gap-2">
              <Upload className="w-4 h-4 text-slate-400 shrink-0" />
              <span className="truncate">{thumbFile ? thumbFile.name : 'Optional Cover Thumbnail'}</span>
              <input
                type="file"
                accept=".jpg,.jpeg,.png,.webp"
                className="hidden"
                onChange={(e) => setThumbFile(e.target.files?.[0] || null)}
              />
            </label>
          </div>

          {uploading && (
            <div className="space-y-1">
              <div className="w-full h-2 rounded-full bg-white/10 overflow-hidden">
                <div className="h-full bg-amber-400 transition-all" style={{ width: `${uploadProgress}%` }} />
              </div>
              <p className="font-mono-num text-xs text-slate-400">Uploading original asset: {uploadProgress}%</p>
            </div>
          )}

          {uploadErr && <p className="text-xs text-rose-300">{uploadErr}</p>}
          {uploadMsg && <p className="text-xs text-emerald-300">{uploadMsg}</p>}

          <button type="submit" disabled={uploading} className="gold-cta px-5 py-2.5 text-xs rounded-xl min-h-[44px] disabled:opacity-60">
            {uploading ? 'Uploading...' : 'Publish to Portfolio'}
          </button>
        </form>
      </div>

      {/* Manage Portfolio Items */}
      <section className="space-y-4">
        <h2 className="font-display text-xl font-bold text-white">Your Portfolio Works ({myPortfolio.length})</h2>
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-5">
          {myPortfolio.map((item) => (
            <div key={item.id} className="glass-panel rounded-2xl overflow-hidden flex flex-col justify-between">
              <div
                onClick={() => onOpenPortfolio(item)}
                className="relative h-44 bg-slate-900 cursor-pointer"
              >
                <img
                  src={item.thumbnail_path || item.file_path}
                  alt={item.title}
                  referrerPolicy="no-referrer"
                  className="w-full h-full object-cover"
                />
                {item.media_type === 'video' && (
                  <div className="absolute top-3 right-3 w-8 h-8 rounded-full bg-black/70 flex items-center justify-center text-amber-400">
                    <Play className="w-3.5 h-3.5 fill-amber-400" />
                  </div>
                )}
              </div>

              <div className="p-4 space-y-3">
                {editingId === item.id ? (
                  <div className="space-y-2">
                    <input
                      type="text"
                      value={editTitle}
                      onChange={(e) => setEditTitle(e.target.value)}
                      className="glass-input w-full rounded-lg px-3 py-1.5 text-xs"
                    />
                    <input
                      type="text"
                      value={editDesc}
                      onChange={(e) => setEditDesc(e.target.value)}
                      className="glass-input w-full rounded-lg px-3 py-1.5 text-xs"
                    />
                    <div className="flex gap-2">
                      <button
                        type="button"
                        onClick={() => handleSavePortfolioEdit(item.id)}
                        className="px-3 py-1.5 text-xs bg-amber-500 text-slate-950 font-semibold rounded-lg"
                      >
                        Save
                      </button>
                      <button
                        type="button"
                        onClick={() => setEditingId(null)}
                        className="px-3 py-1.5 text-xs text-slate-300"
                      >
                        Cancel
                      </button>
                    </div>
                  </div>
                ) : (
                  <>
                    <div>
                      <p className="text-xs text-amber-300">{item.category} · {item.media_type}</p>
                      <h3 className="font-display text-base font-semibold text-white">{item.title}</h3>
                      <p className="text-xs text-slate-400 line-clamp-2 mt-1">{item.description}</p>
                    </div>
                    <div className="flex items-center justify-between pt-2 border-t border-white/[0.08]">
                      <button
                        type="button"
                        onClick={() => {
                          setEditingId(item.id);
                          setEditTitle(item.title);
                          setEditDesc(item.description);
                        }}
                        className="inline-flex items-center gap-1 text-xs text-slate-300 hover:text-white min-h-[36px]"
                      >
                        <Pencil className="w-3.5 h-3.5" />
                        <span>Edit</span>
                      </button>
                      <button
                        type="button"
                        onClick={() => handleDeletePortfolio(item.id)}
                        className="inline-flex items-center gap-1 text-xs text-rose-300 hover:text-rose-200 min-h-[36px]"
                      >
                        <Trash2 className="w-3.5 h-3.5" />
                        <span>Delete</span>
                      </button>
                    </div>
                  </>
                )}
              </div>
            </div>
          ))}
        </div>
      </section>
    </div>
  );
};
