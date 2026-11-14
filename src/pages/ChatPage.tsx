import React, { useEffect, useRef, useState } from 'react';
import { ArrowLeft, Send } from 'lucide-react';
import { api } from '../api/client';
import type { ChatMessage, Conversation, User } from '../types';

interface ChatPageProps {
  user: User;
  initialPeerUserId?: number | null;
}

export const ChatPage: React.FC<ChatPageProps> = ({ user, initialPeerUserId }) => {
  const [conversations, setConversations] = useState<Conversation[]>([]);
  const [activePeerId, setActivePeerId] = useState<number | null>(initialPeerUserId || null);
  const [messages, setMessages] = useState<ChatMessage[]>([]);
  const [draft, setDraft] = useState('');
  const [sending, setSending] = useState(false);
  const endRef = useRef<HTMLDivElement | null>(null);

  const loadConversations = async () => {
    try {
      const res = await api.getConversations();
      setConversations(res.conversations);
      if (!activePeerId && !initialPeerUserId && res.conversations.length > 0) {
        setActivePeerId(res.conversations[0].user_id);
      }
    } catch {
      /* ignore */
    }
  };

  const loadMessages = async (peerId: number) => {
    try {
      const res = await api.getMessages(peerId);
      setMessages(res.messages);
    } catch {
      /* ignore */
    }
  };

  useEffect(() => {
    loadConversations();
  }, [user.id]);

  useEffect(() => {
    if (initialPeerUserId) setActivePeerId(initialPeerUserId);
  }, [initialPeerUserId]);

  useEffect(() => {
    if (!activePeerId) return;
    loadMessages(activePeerId);
    const interval = setInterval(() => {
      loadMessages(activePeerId);
      loadConversations();
    }, 4000);
    return () => clearInterval(interval);
  }, [activePeerId]);

  useEffect(() => {
    endRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [messages.length]);

  const handleSend = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!activePeerId || !draft.trim()) return;
    setSending(true);
    try {
      await api.sendMessage({ receiver_id: activePeerId, content: draft.trim() });
      setDraft('');
      await loadMessages(activePeerId);
      await loadConversations();
    } finally {
      setSending(false);
    }
  };

  const activeConv = conversations.find((c) => c.user_id === activePeerId);

  return (
    <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-6">
      <div className="glass-panel rounded-3xl overflow-hidden grid grid-cols-1 md:grid-cols-12 min-h-[70vh]">
        {/* Conversations Sidebar */}
        <aside
          className={`md:col-span-4 border-r border-white/[0.08] flex flex-col ${
            activePeerId ? 'hidden md:flex' : 'flex'
          }`}
        >
          <div className="p-5 border-b border-white/[0.08]">
            <h1 className="font-display text-xl font-bold text-white">CUTCRAFT Messages</h1>
            <p className="text-xs text-slate-400 mt-0.5">Direct client-freelancer studio threads</p>
          </div>
          <div className="divide-y divide-white/[0.06] overflow-y-auto flex-1">
            {conversations.length === 0 ? (
              <p className="p-6 text-sm text-slate-400">
                No conversations yet. Open any creator profile and click Message to start chatting.
              </p>
            ) : (
              conversations.map((conv) => (
                <button
                  key={conv.user_id}
                  type="button"
                  onClick={() => setActivePeerId(conv.user_id)}
                  className={`w-full p-4 text-left flex items-center gap-3 transition-colors min-h-[64px] ${
                    activePeerId === conv.user_id ? 'bg-amber-500/10' : 'hover:bg-white/[0.04]'
                  }`}
                >
                  <img
                    src={conv.avatar_url || '/assets/generated/creator_portrait_designer.jpg'}
                    alt={conv.full_name}
                    referrerPolicy="no-referrer"
                    className="w-11 h-11 rounded-full object-cover shrink-0 border border-white/15"
                  />
                  <div className="min-w-0 flex-1">
                    <div className="flex items-center justify-between gap-2">
                      <p className="text-sm font-semibold text-white truncate">{conv.full_name}</p>
                      <span className="text-[11px] text-slate-500 font-mono-num shrink-0">
                        {conv.last_timestamp ? conv.last_timestamp.slice(11, 16) : ''}
                      </span>
                    </div>
                    <p className="text-xs text-amber-300 truncate">{conv.subtitle}</p>
                    <p className="text-xs text-slate-400 truncate mt-0.5">{conv.last_message || 'Start conversation'}</p>
                  </div>
                </button>
              ))
            )}
          </div>
        </aside>

        {/* Active Message Thread */}
        <section
          className={`md:col-span-8 flex flex-col justify-between ${
            activePeerId ? 'flex' : 'hidden md:flex'
          }`}
        >
          {!activePeerId ? (
            <div className="flex-1 flex items-center justify-center p-8 text-sm text-slate-400">
              Select a conversation to view messages.
            </div>
          ) : (
            <>
              <div className="px-5 py-4 border-b border-white/[0.08] flex items-center gap-3">
                <button
                  type="button"
                  onClick={() => setActivePeerId(null)}
                  className="md:hidden min-w-[40px] min-h-[40px] flex items-center justify-center text-slate-300"
                  aria-label="Back to conversations"
                >
                  <ArrowLeft className="w-5 h-5" />
                </button>
                <div>
                  <h2 className="font-display text-base font-semibold text-white">
                    {activeConv?.full_name || `User #${activePeerId}`}
                  </h2>
                  <p className="text-xs text-slate-400">{activeConv?.subtitle || 'CUTCRAFT Member'}</p>
                </div>
              </div>

              <div className="flex-1 p-5 space-y-3 overflow-y-auto max-h-[52vh]">
                {messages.length === 0 ? (
                  <p className="text-sm text-slate-400">No messages yet. Say hello to begin collaborating.</p>
                ) : (
                  messages.map((m) => {
                    const isMe = m.sender_id === user.id;
                    return (
                      <div key={m.id} className={`flex flex-col ${isMe ? 'items-end' : 'items-start'}`}>
                        <div
                          className={`max-w-[80%] rounded-2xl px-4 py-2.5 text-sm leading-relaxed ${
                            isMe
                              ? 'bg-amber-500 text-slate-950 font-medium'
                              : 'bg-white/[0.07] border border-white/[0.10] text-slate-100'
                          }`}
                        >
                          {m.content}
                        </div>
                        <span className="text-[11px] text-slate-500 mt-1 font-mono-num">
                          {m.sender_name} · {m.created_at}
                        </span>
                      </div>
                    );
                  })
                )}
                <div ref={endRef} />
              </div>

              <form onSubmit={handleSend} className="p-4 border-t border-white/[0.08] flex gap-3">
                <input
                  type="text"
                  value={draft}
                  onChange={(e) => setDraft(e.target.value)}
                  placeholder="Write a message..."
                  className="glass-input flex-1 rounded-xl px-4 py-3 text-sm"
                />
                <button
                  type="submit"
                  disabled={sending || !draft.trim()}
                  className="gold-cta px-5 py-3 rounded-xl inline-flex items-center gap-2 text-xs min-h-[44px] disabled:opacity-50"
                >
                  <Send className="w-4 h-4" />
                  <span>Send</span>
                </button>
              </form>
            </>
          )}
        </section>
      </div>
    </div>
  );
};
