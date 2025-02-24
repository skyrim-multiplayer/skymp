export interface RPCResponse {
  rpcFound: boolean;
  rpcResult: Record<string, unknown> | null;
  rpcException: string | Error | null; // not sure about that type
};
