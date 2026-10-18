/**
 * Admin Dashboard WebSocket Client Hook
 * Manages real-time updates from /ws/admin-updates
 * Provides auto-reconnect with exponential backoff
 */

import { useEffect, useRef, useState, useCallback } from 'react';

export interface AdminWebSocketMessage {
  type: 'connected' | 'status' | 'players' | 'log' | 'event' | 'history' | 'respawn';
  ts: number;
  data: any;
}

export interface AdminWebSocketState {
  connected: boolean;
  reconnecting: boolean;
  messageCount: number;
  lastMessageTs: number | null;
  error: string | null;
}

const DEFAULT_RECONNECT_DELAY = 1000;
const MAX_RECONNECT_DELAY = 30000;

/**
 * Hook to manage WebSocket connection for admin updates
 * Automatically reconnects with exponential backoff if connection fails
 * Falls back gracefully if WebSocket is unavailable
 */
export const useAdminWebSocket = (
  onMessage: (message: AdminWebSocketMessage) => void,
  enabled: boolean = true,
): AdminWebSocketState => {
  const [state, setState] = useState<AdminWebSocketState>({
    connected: false,
    reconnecting: false,
    messageCount: 0,
    lastMessageTs: null,
    error: null,
  });

  const wsRef = useRef<WebSocket | null>(null);
  const reconnectTimeoutRef = useRef<NodeJS.Timeout | null>(null);
  const reconnectDelayRef = useRef<number>(DEFAULT_RECONNECT_DELAY);
  const closedByUserRef = useRef<boolean>(false);
  const onMessageRef = useRef(onMessage);

  useEffect(() => {
    onMessageRef.current = onMessage;
  }, [onMessage]);

  const connect = useCallback(() => {
    if (!enabled || closedByUserRef.current) return;

    setState((prev) => ({ ...prev, reconnecting: true, error: null }));

    try {
      const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
      const wsUrl = `${protocol}//${window.location.host}/ws/admin-updates`;

      const ws = new WebSocket(wsUrl);
      wsRef.current = ws;

      ws.onopen = () => {
        if (wsRef.current !== ws) return;
        reconnectDelayRef.current = DEFAULT_RECONNECT_DELAY;
        setState((prev) => ({
          ...prev,
          connected: true,
          reconnecting: false,
          error: null,
        }));
      };

      ws.onmessage = (event) => {
        if (wsRef.current !== ws) return;
        try {
          const message = JSON.parse(event.data) as AdminWebSocketMessage;
          setState((prev) => ({
            ...prev,
            messageCount: prev.messageCount + 1,
            lastMessageTs: Date.now(),
          }));
          onMessageRef.current(message);
        } catch (error) {
          console.error('[AdminWebSocket] Error parsing message:', error);
        }
      };

      ws.onerror = () => {
        if (wsRef.current !== ws) return;
        setState((prev) => ({
          ...prev,
          connected: false,
          error: 'WebSocket error',
        }));
      };

      ws.onclose = () => {
        if (wsRef.current !== ws) return;
        wsRef.current = null;
        setState((prev) => ({
          ...prev,
          connected: false,
        }));

        // Auto-reconnect with exponential backoff
        if (!closedByUserRef.current && enabled) {
          setState((prev) => ({ ...prev, reconnecting: true }));
          reconnectTimeoutRef.current = setTimeout(() => {
            reconnectDelayRef.current = Math.min(
              reconnectDelayRef.current * 1.5,
              MAX_RECONNECT_DELAY,
            );
            connect();
          }, reconnectDelayRef.current);
        }
      };
    } catch (error) {
      console.error('[AdminWebSocket] Error connecting:', error);
      setState((prev) => ({
        ...prev,
        error: String(error),
        reconnecting: false,
      }));
    }
  }, [enabled]);

  const disconnect = useCallback(() => {
    closedByUserRef.current = true;
    if (reconnectTimeoutRef.current) {
      clearTimeout(reconnectTimeoutRef.current);
    }
    if (wsRef.current) {
      const ws = wsRef.current;
      wsRef.current = null;
      ws.close();
    }
    setState({
      connected: false,
      reconnecting: false,
      messageCount: 0,
      lastMessageTs: null,
      error: null,
    });
  }, []);

  // Connect on mount
  useEffect(() => {
    if (!enabled) return;

    closedByUserRef.current = false;
    connect();

    return () => {
      disconnect();
    };
  }, [enabled, connect, disconnect]);

  return state;
};

/**
 * Helper hook to listen for specific message types
 */
export const useAdminWebSocketMessages = (
  messageType: AdminWebSocketMessage['type'],
  onMessage: (data: any) => void,
  enabled: boolean = true,
): AdminWebSocketState => {
  const handleMessage = useCallback(
    (message: AdminWebSocketMessage) => {
      if (message.type === messageType) {
        onMessage(message.data);
      }
    },
    [messageType, onMessage],
  );

  return useAdminWebSocket(handleMessage, enabled);
};
