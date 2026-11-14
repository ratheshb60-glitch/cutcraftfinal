export type AccountType = 'client' | 'freelancer';

export interface User {
  id: number;
  full_name: string;
  email: string;
  account_type: AccountType;
  avatar_url: string;
  freelancer_profile_id: number;
}

export interface Freelancer {
  id: number;
  user_id: number;
  full_name: string;
  avatar_url: string;
  profession: string;
  bio: string;
  skills: string;
  starting_price: number;
  location: string;
  rating_avg: number;
  review_count: number;
  completed_count: number;
  is_featured: number;
  cover_url: string;
}

export interface PortfolioItem {
  id: number;
  freelancer_id: number;
  title: string;
  description: string;
  category: string;
  media_type: 'image' | 'video';
  file_path: string;
  thumbnail_path: string;
  original_filename?: string;
  file_size_bytes?: number;
  mime_type?: string;
  storage_provider?: string;
  r2_object_key?: string;
  mux_asset_id?: string;
  mux_playback_id?: string;
  is_trending?: number;
  created_at?: string;
  creator_name?: string;
  creator_avatar?: string;
  profession?: string;
  starting_price?: number;
}

export interface Review {
  id: number;
  project_id: number;
  rating: number;
  comment: string;
  created_at: string;
  client_name: string;
  client_avatar: string;
  project_title: string;
}

export interface ProjectRequest {
  id: number;
  client_id: number;
  freelancer_id: number;
  title: string;
  description: string;
  budget: number;
  commission_rate: number;
  platform_fee: number;
  freelancer_earnings: number;
  deadline: string;
  attachment_path: string;
  status: 'pending' | 'accepted' | 'rejected';
  created_at: string;
  client_name: string;
  client_avatar: string;
  freelancer_name: string;
  freelancer_profession: string;
  freelancer_user_id: number;
}

export interface Project {
  id: number;
  client_id: number;
  freelancer_id: number;
  title: string;
  description: string;
  budget: number;
  commission_rate: number;
  platform_fee: number;
  freelancer_earnings: number;
  deadline: string;
  status: 'accepted' | 'in_progress' | 'completed' | 'cancelled';
  progress_percent: number;
  payment_status: 'unpaid' | 'simulated_paid';
  created_at: string;
  updated_at: string;
  client_name: string;
  client_avatar: string;
  freelancer_name: string;
  freelancer_profession: string;
  freelancer_user_id: number;
  has_review: number;
}

export interface Conversation {
  user_id: number;
  full_name: string;
  account_type: AccountType;
  avatar_url: string;
  subtitle: string;
  last_message: string;
  last_timestamp: string;
}

export interface ChatMessage {
  id: number;
  sender_id: number;
  receiver_id: number;
  project_id: number;
  content: string;
  is_read: number;
  created_at: string;
  sender_name: string;
}

export interface CommissionBreakdown {
  budget: number;
  commission_rate: number;
  platform_earnings: number;
  freelancer_earnings: number;
  is_simulated?: boolean;
}

export interface TransactionRecord {
  id: number;
  project_id: number;
  project_title: string;
  gross_amount: number;
  commission_rate: number;
  platform_earnings: number;
  freelancer_earnings: number;
  payment_method: string;
  payment_status: string;
  reference_code: string;
  created_at: string;
}

export interface EarningsSummary {
  commission_rate: number;
  total_gross: number;
  total_commission: number;
  total_net_earnings: number;
  transaction_count: number;
  transactions: TransactionRecord[];
}

export type PageRoute =
  | 'landing'
  | 'register'
  | 'login'
  | 'discover'
  | 'profile'
  | 'dashboard'
  | 'request'
  | 'chat'
  | 'projects'
  | 'settings'
  | 'about';
