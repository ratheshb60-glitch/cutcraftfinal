import React, { useState } from 'react';
import { Pencil, RotateCcw, Upload, User as UserIcon } from 'lucide-react';
import { CUTCRAFT_LOGO_SRC } from '../components/Navbar';

interface DeveloperSlot {
  id: number;
  name: string;
  role: string;
  responsibility: string;
  photoUrl: string;
}

interface CollegeMeta {
  collegeName: string;
  department: string;
  projectTitle: string;
  appVersion: string;
}

const DEFAULT_META: CollegeMeta = {
  collegeName: '[College / University Name Placeholder]',
  department: '[Department of Computer Science / Engineering Placeholder]',
  projectTitle: 'CUTCRAFT — Creative Freelancer Marketplace (C & SQLite)',
  appVersion: 'v1.0.0',
};

const DEFAULT_DEVELOPERS: DeveloperSlot[] = [
  {
    id: 1,
    name: '[Developer 1 Name]',
    role: '[Role Placeholder: Backend & SQLite Lead]',
    responsibility: '[Responsibilities: C HTTP Server, SQLite Schema, Authentication & Commission Modules]',
    photoUrl: '',
  },
  {
    id: 2,
    name: '[Developer 2 Name]',
    role: '[Role Placeholder: Frontend & Liquid Glass UI]',
    responsibility: '[Responsibilities: Responsive Design, Portfolio Gallery, Client & Freelancer Workflows]',
    photoUrl: '',
  },
  {
    id: 3,
    name: '[Developer 3 Name]',
    role: '[Role Placeholder: Integration & Testing]',
    responsibility: '[Responsibilities: Search/Filter Integration, Messaging, File Upload Testing & Deployment]',
    photoUrl: '',
  },
  {
    id: 4,
    name: '[Developer 4 Name]',
    role: '[Role Placeholder: Database & Security]',
    responsibility: '[Responsibilities: SQLite Schema Design, Input Validation, Parameterized Queries & Session Handling]',
    photoUrl: '',
  },
  {
    id: 5,
    name: '[Developer 5 Name]',
    role: '[Role Placeholder: Media & Documentation]',
    responsibility: '[Responsibilities: Portfolio Media Pipeline, Responsive QA, Project Documentation & Presentation]',
    photoUrl: '',
  },
];

const STORAGE_KEY = 'cutcraft_about_developers_v1';

export const AboutDevelopersPage: React.FC = () => {
  const saved = (() => {
    try {
      const raw = localStorage.getItem(STORAGE_KEY);
      return raw ? JSON.parse(raw) : null;
    } catch {
      return null;
    }
  })();

  const initialDevelopers: DeveloperSlot[] = (() => {
    if (Array.isArray(saved?.developers) && saved.developers.length > 0) {
      if (saved.developers.length >= DEFAULT_DEVELOPERS.length) return saved.developers;
      const existingIds = new Set(saved.developers.map((d: DeveloperSlot) => d.id));
      return [
        ...saved.developers,
        ...DEFAULT_DEVELOPERS.filter((d) => !existingIds.has(d.id)),
      ];
    }
    return DEFAULT_DEVELOPERS;
  })();

  const [meta, setMeta] = useState<CollegeMeta>(saved?.meta || DEFAULT_META);
  const [developers, setDevelopers] = useState<DeveloperSlot[]>(initialDevelopers);
  const [editing, setEditing] = useState(false);

  const persist = (nextMeta: CollegeMeta, nextDevs: DeveloperSlot[]) => {
    setMeta(nextMeta);
    setDevelopers(nextDevs);
    localStorage.setItem(STORAGE_KEY, JSON.stringify({ meta: nextMeta, developers: nextDevs }));
  };

  const handleReset = () => {
    localStorage.removeItem(STORAGE_KEY);
    setMeta(DEFAULT_META);
    setDevelopers(DEFAULT_DEVELOPERS);
  };

  const handlePhotoSelect = (id: number, file: File) => {
    const reader = new FileReader();
    reader.onload = () => {
      const result = typeof reader.result === 'string' ? reader.result : '';
      const updated = developers.map((d) => (d.id === id ? { ...d, photoUrl: result } : d));
      persist(meta, updated);
    };
    reader.readAsDataURL(file);
  };

  return (
    <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-12 space-y-10">
      {/* Project Header */}
      <div className="glass-panel rounded-3xl p-6 sm:p-8 flex flex-col lg:flex-row lg:items-center justify-between gap-6">
        <div className="flex items-center gap-4">
          <img
            src={CUTCRAFT_LOGO_SRC}
            alt="CUTCRAFT logo"
            referrerPolicy="no-referrer"
            className="w-16 h-16 rounded-full object-cover border border-amber-400/50 shrink-0"
          />
          <div>
            <p className="text-xs text-amber-400 font-medium">About the Developers</p>
            <h1 className="font-display text-2xl sm:text-3xl font-bold text-white">CUTCRAFT</h1>
            <p className="text-sm text-slate-300 mt-0.5">Where Creativity Meets Opportunity.</p>
          </div>
        </div>

        <div className="flex flex-wrap gap-3">
          <button
            type="button"
            onClick={() => setEditing(!editing)}
            className="gold-cta px-4 py-2.5 text-xs rounded-xl inline-flex items-center gap-1.5 min-h-[44px]"
          >
            <Pencil className="w-3.5 h-3.5" />
            <span>{editing ? 'Done Editing Placeholders' : 'Edit Developer Placeholders'}</span>
          </button>
          <button
            type="button"
            onClick={handleReset}
            className="px-4 py-2.5 text-xs font-medium rounded-xl border border-white/15 text-slate-300 hover:text-white inline-flex items-center gap-1.5 min-h-[44px]"
          >
            <RotateCcw className="w-3.5 h-3.5" />
            <span>Reset Placeholders</span>
          </button>
        </div>
      </div>

      {/* Institutional & Project Metadata */}
      <div className="glass-panel rounded-2xl p-6 grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
        <div>
          <p className="text-xs text-slate-400">Project Title</p>
          {editing ? (
            <input
              type="text"
              value={meta.projectTitle}
              onChange={(e) => persist({ ...meta, projectTitle: e.target.value }, developers)}
              className="glass-input w-full rounded-lg px-3 py-1.5 text-sm mt-1"
            />
          ) : (
            <p className="text-sm font-semibold text-white mt-1">{meta.projectTitle}</p>
          )}
        </div>
        <div>
          <p className="text-xs text-slate-400">College / Institution</p>
          {editing ? (
            <input
              type="text"
              value={meta.collegeName}
              onChange={(e) => persist({ ...meta, collegeName: e.target.value }, developers)}
              className="glass-input w-full rounded-lg px-3 py-1.5 text-sm mt-1"
            />
          ) : (
            <p className="text-sm font-semibold text-white mt-1">{meta.collegeName}</p>
          )}
        </div>
        <div>
          <p className="text-xs text-slate-400">Department</p>
          {editing ? (
            <input
              type="text"
              value={meta.department}
              onChange={(e) => persist({ ...meta, department: e.target.value }, developers)}
              className="glass-input w-full rounded-lg px-3 py-1.5 text-sm mt-1"
            />
          ) : (
            <p className="text-sm font-semibold text-white mt-1">{meta.department}</p>
          )}
        </div>
        <div>
          <p className="text-xs text-slate-400">App Version &amp; Stack</p>
          <p className="font-mono-num text-sm font-semibold text-amber-400 mt-1">
            {meta.appVersion} · C11 + SQLite3 + React
          </p>
        </div>
      </div>

      {/* Developer Placeholder Cards (5 Slots) */}
      <section className="space-y-4">
        <h2 className="font-display text-2xl font-bold text-white">Development Team (Editable Placeholders)</h2>
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
          {developers.map((dev) => (
            <div key={dev.id} className="glass-panel rounded-2xl p-6 space-y-4">
              <div className="flex items-center gap-4">
                <div className="w-24 h-24 rounded-2xl bg-white/[0.04] border border-white/15 flex items-center justify-center overflow-hidden shrink-0">
                  {dev.photoUrl ? (
                    <img
                      src={dev.photoUrl}
                      alt={dev.name}
                      referrerPolicy="no-referrer"
                      className="w-full h-full object-cover"
                    />
                  ) : (
                    <div className="text-center p-2">
                      <UserIcon className="w-7 h-7 text-slate-500 mx-auto" />
                      <span className="text-[10px] text-slate-500 block mt-1">Photo Slot</span>
                    </div>
                  )}
                </div>

                <div className="min-w-0 flex-1 space-y-1">
                  {editing ? (
                    <>
                      <input
                        type="text"
                        value={dev.name}
                        onChange={(e) =>
                          persist(
                            meta,
                            developers.map((d) => (d.id === dev.id ? { ...d, name: e.target.value } : d))
                          )
                        }
                        className="glass-input w-full rounded-lg px-2.5 py-1 text-sm"
                      />
                      <input
                        type="text"
                        value={dev.role}
                        onChange={(e) =>
                          persist(
                            meta,
                            developers.map((d) => (d.id === dev.id ? { ...d, role: e.target.value } : d))
                          )
                        }
                        className="glass-input w-full rounded-lg px-2.5 py-1 text-xs"
                      />
                    </>
                  ) : (
                    <>
                      <h3 className="font-display text-lg font-semibold text-white truncate">{dev.name}</h3>
                      <p className="text-xs text-amber-300">{dev.role}</p>
                    </>
                  )}
                </div>
              </div>

              {editing ? (
                <div className="space-y-2">
                  <textarea
                    rows={2}
                    value={dev.responsibility}
                    onChange={(e) =>
                      persist(
                        meta,
                        developers.map((d) => (d.id === dev.id ? { ...d, responsibility: e.target.value } : d))
                      )
                    }
                    className="glass-input w-full rounded-lg px-3 py-2 text-xs"
                  />
                  <label className="inline-flex items-center gap-1.5 text-xs text-amber-400 cursor-pointer">
                    <Upload className="w-3.5 h-3.5" />
                    <span>Insert Developer Photograph</span>
                    <input
                      type="file"
                      accept="image/*"
                      className="hidden"
                      onChange={(e) => {
                        const f = e.target.files?.[0];
                        if (f) handlePhotoSelect(dev.id, f);
                      }}
                    />
                  </label>
                </div>
              ) : (
                <p className="text-sm text-slate-300 leading-relaxed">{dev.responsibility}</p>
              )}
            </div>
          ))}
        </div>
      </section>

      {/* Honest Technical Limitations & Architecture Notes */}
      <div className="glass-panel rounded-2xl p-6 space-y-2">
        <h2 className="font-display text-lg font-semibold text-white">Architecture &amp; Technical Notes</h2>
        <p className="text-xs text-slate-300 leading-relaxed">
          CUTCRAFT runs its authentication, salted SHA-256 hashing, freelancer search, project request workflow, chat messaging, and commission math inside compiled C11 modules (`server.c`, `authentication.c`, `user.c`, `freelancer.c`, `search.c`, `projects.c`, `hiring.c`, `commission.c`, `database.c`) backed by SQLite3. Original uploaded photos and videos are stored uncompressed on disk, with schema hooks prepared for Cloudflare R2 object storage and Mux adaptive HLS streaming. All payment flows in v1 are simulated escrow records—no live financial gateway is connected yet.
        </p>
        <p className="text-xs text-slate-500 pt-2">
          Copyright © 2026 CUTCRAFT. All rights reserved.
        </p>
      </div>
    </div>
  );
};
