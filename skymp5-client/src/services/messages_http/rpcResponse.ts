export interface RPCResponse<RPCResult = Record<string, unknown>> {
    rpcFound: boolean;
    rpcResult: RPCResult | null;
    rpcException: string | null;
}
