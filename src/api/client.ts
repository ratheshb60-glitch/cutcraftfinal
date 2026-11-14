import type {
  CommissionBreakdown,
  Conversation,
  EarningsSummary,
  Freelancer,
  PortfolioItem,
  Project,
  ProjectRequest,
  Review,
  ChatMessage,
  User,
} from '../types';

const TOKEN_KEY = 'cutcraft_token';

export function getStoredToken(): string | null {
  return localStorage.getItem(TOKEN_KEY);
}

export function setStoredToken(token: string | null) {
  if (token) {
    localStorage.setItem(TOKEN_KEY, token);
  } else {
    localStorage.removeItem(TOKEN_KEY);
  }
}

async function apiFetch<T>(path: string, options: RequestInit = {}): Promise<T> {
  const token = getStoredToken();
  const headers: Record<string, string> = {
    'Content-Type': 'application/json',
    ...(options.headers as Record<string, string> | undefined),
  };
  if (token) {
    headers['Authorization'] = `Bearer ${token}`;
  }

  const res = await fetch(path, { ...options, headers });
  const data = await res.json().catch(() => ({}));
  if (!res.ok) {
    throw new Error((data && data.error) || `Request failed (${res.status})`);
  }
  return data as T;
}

export interface UploadResult {
  file_path: string;
  original_filename: string;
  file_size_bytes: number;
  mime_type: string;
  media_type: 'image' | 'video' | 'document';
  storage_provider: string;
  r2_ready: boolean;
  mux_ready: boolean;
}

export function uploadOriginalFile(
  file: File,
  onProgress?: (percent: number) => void
): Promise<UploadResult> {
  return new Promise((resolve, reject) => {
    const token = getStoredToken();
    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/api/upload', true);
    if (token) xhr.setRequestHeader('Authorization', `Bearer ${token}`);
    xhr.setRequestHeader('X-Filename', file.name);
    xhr.setRequestHeader('Content-Type', file.type || 'application/octet-stream');

    xhr.upload.onprogress = (evt) => {
      if (evt.lengthComputable && onProgress) {
        onProgress(Math.round((evt.loaded / evt.total) * 100));
      }
    };

    xhr.onload = () => {
      try {
        const parsed = JSON.parse(xhr.responseText || '{}');
        if (xhr.status >= 200 && xhr.status < 300) {
          resolve(parsed as UploadResult);
        } else {
          reject(new Error(parsed.error || `Upload failed (${xhr.status})`));
        }
      } catch {
        reject(new Error('Invalid upload response from server.'));
      }
    };

    xhr.onerror = () => reject(new Error('Network error during upload.'));
    xhr.send(file);
  });
}

export const api = {
  register: (payload: {
    full_name: string;
    email: string;
    password: string;
    account_type: 'client' | 'freelancer';
    profession?: string;
  }) => apiFetch<{ token: string; user: User }>('/api/auth/register', { method: 'POST', body: JSON.stringify(payload) }),

  login: (payload: { email: string; password: string }) =>
    apiFetch<{ token: string; user: User }>('/api/auth/login', { method: 'POST', body: JSON.stringify(payload) }),

  logout: () => apiFetch<{ ok: boolean }>('/api/auth/logout', { method: 'POST', body: '{}' }),

  me: () => apiFetch<{ user: User }>('/api/auth/me'),

  forgotPassword: (email: string) =>
    apiFetch<{ message: string }>('/api/auth/forgot-password', { method: 'POST', body: JSON.stringify({ email }) }),

  changePassword: (current_password: string, new_password: string) =>
    apiFetch<{ message: string }>('/api/auth/change-password', {
      method: 'POST',
      body: JSON.stringify({ current_password, new_password }),
    }),

  updateUserProfile: (payload: { full_name?: string; avatar_url?: string }) =>
    apiFetch<{ user: User }>('/api/users/profile', { method: 'PUT', body: JSON.stringify(payload) }),

  getFeaturedFreelancers: () => apiFetch<{ freelancers: Freelancer[] }>('/api/freelancers/featured'),

  getAllFreelancers: () => apiFetch<{ freelancers: Freelancer[] }>('/api/freelancers'),

  searchFreelancers: (params: {
    q?: string;
    category?: string;
    min_price?: string;
    max_price?: string;
    min_rating?: string;
    sort?: string;
  }) => {
    const sp = new URLSearchParams();
    Object.entries(params).forEach(([k, v]) => {
      if (v) sp.set(k, v);
    });
    return apiFetch<{ freelancers: Freelancer[] }>(`/api/freelancers/search?${sp.toString()}`);
  },

  getFreelancerDetail: (id: number) =>
    apiFetch<{ freelancer: Freelancer; portfolio: PortfolioItem[]; reviews: Review[] }>(`/api/freelancers/${id}`),

  upsertFreelancerProfile: (payload: {
    profession: string;
    bio: string;
    skills: string;
    starting_price: number;
    location: string;
  }) =>
    apiFetch<{ freelancer: Freelancer; portfolio: PortfolioItem[]; reviews: Review[] }>('/api/freelancers/profile', {
      method: 'PUT',
      body: JSON.stringify(payload),
    }),

  getTrendingPortfolios: () => apiFetch<{ portfolios: PortfolioItem[] }>('/api/portfolios/trending'),

  addPortfolioItem: (payload: {
    title: string;
    description: string;
    category: string;
    media_type: 'image' | 'video';
    file_path: string;
    thumbnail_path?: string;
    original_filename?: string;
    file_size_bytes?: number;
    mime_type?: string;
    r2_object_key?: string;
    mux_asset_id?: string;
  }) => apiFetch<{ id: number; message: string }>('/api/portfolios', { method: 'POST', body: JSON.stringify(payload) }),

  updatePortfolioItem: (id: number, payload: { title: string; description: string; category?: string }) =>
    apiFetch<{ ok: boolean }>(`/api/portfolios/${id}`, { method: 'PUT', body: JSON.stringify(payload) }),

  deletePortfolioItem: (id: number) => apiFetch<{ deleted: boolean }>(`/api/portfolios/${id}`, { method: 'DELETE' }),

  createProjectRequest: (payload: {
    freelancer_id: number;
    title: string;
    description: string;
    budget: number;
    deadline: string;
    attachment_path?: string;
  }) =>
    apiFetch<{
      id: number;
      status: string;
      commission_rate: number;
      platform_fee: number;
      freelancer_earnings: number;
      message: string;
    }>('/api/requests', { method: 'POST', body: JSON.stringify(payload) }),

  getProjectRequests: () => apiFetch<{ requests: ProjectRequest[] }>('/api/requests'),

  respondProjectRequest: (id: number, action: 'accept' | 'reject') =>
    apiFetch<{ status: string; project_id: number }>(`/api/requests/${id}/${action}`, { method: 'POST', body: '{}' }),

  getProjects: () => apiFetch<{ projects: Project[] }>('/api/projects'),

  updateProjectStatus: (id: number, payload: { status?: string; progress_percent?: number }) =>
    apiFetch<{ id: number; status: string; progress_percent: number }>(`/api/projects/${id}`, {
      method: 'PATCH',
      body: JSON.stringify(payload),
    }),

  payProjectSimulated: (id: number, payment_method: string) =>
    apiFetch<{
      payment_status: string;
      is_simulated: boolean;
      notice: string;
      reference_code: string;
      gross_amount: number;
      commission_rate: number;
      platform_earnings: number;
      freelancer_earnings: number;
    }>(`/api/projects/${id}/pay`, { method: 'POST', body: JSON.stringify({ payment_method }) }),

  submitReview: (projectId: number, payload: { rating: number; comment: string }) =>
    apiFetch<{ ok: boolean; message: string }>(`/api/projects/${projectId}/review`, {
      method: 'POST',
      body: JSON.stringify(payload),
    }),

  getConversations: () => apiFetch<{ conversations: Conversation[] }>('/api/conversations'),

  getMessages: (peerUserId: number) => apiFetch<{ messages: ChatMessage[] }>(`/api/messages/${peerUserId}`),

  sendMessage: (payload: { receiver_id: number; content: string; project_id?: number }) =>
    apiFetch<{ id: number; sender_id: number; receiver_id: number }>('/api/messages', {
      method: 'POST',
      body: JSON.stringify(payload),
    }),

  previewCommission: (budget: number) =>
    apiFetch<CommissionBreakdown>(`/api/commission/preview?budget=${encodeURIComponent(String(budget))}`),

  updateCommissionRate: (commission_rate: number) =>
    apiFetch<{ commission_rate: number; message: string }>('/api/commission/rate', {
      method: 'PUT',
      body: JSON.stringify({ commission_rate }),
    }),

  getEarnings: () => apiFetch<EarningsSummary>('/api/earnings'),
};
