import * as WebSocket from 'ws';
import * as scampNative from './scampNative';
import { Settings } from './settings';

export type OnUiEvent = (formId: number, msg: Record<string, unknown>) => void;

const tokenByUserId = new Array<string | undefined>();
let clients = new Array<Record<string, unknown>>();
let onUiEvent: OnUiEvent = () => undefined;
let clientByUserId = new Array<undefined | Record<string, unknown>>();

clientByUserId.length = 2000; // Hard user limit

export const onBrowserTokenChange = (userId: number, newToken: string): void => {
  // This cleanup actually matters when players reconnect:
  // userIds may change in this case, while tokens remain the same
  const previousUserId = tokenByUserId.findIndex((x) => x === newToken);
  if (previousUserId !== -1) {
    tokenByUserId[previousUserId] = undefined;
  }

  if (tokenByUserId.length <= userId) {
    tokenByUserId.length = userId + 1;
  }
  tokenByUserId[userId] = newToken;
};

const distance = (pos1: number[], pos2: number[]): number => {
  const a = pos1[0] - pos2[0];
  const b = pos1[1] - pos2[1];
  const c = pos1[2] - pos2[2];
  return Math.sqrt(a * a + b * b + c * c);
};

const getUserActorOrZero = (server: scampNative.ScampServer, userId: number) => {
  try {
    return server.getUserActor(userId);
  } catch (e) {
    return 0;
  }
};

export const sendMsg = (server: scampNative.ScampServer, formId: number, msg: Record<string, unknown>): void => {
  const userId = server.getUserByActor(formId);
  const invalidUserId = 65535;
  if (userId === invalidUserId) return;

  const ws = clientByUserId[userId];
  if (!ws) return;

  (ws as WebSocket).send(
    JSON.stringify({
      message: msg,
    }),
  );
};

export const attachMpApi = (_onUiEvent: OnUiEvent): void => {
  onUiEvent = _onUiEvent;
};

export const main = (server: scampNative.ScampServer): void => {
  const svr = new WebSocket.Server({
    port: Settings.get().port === 7777 ? 8080 : Settings.get().port + 2,
  });
  console.log('websocket server up');

  svr.on('close', (ws: Record<string, unknown>) => {
    clients = clients.filter((cl) => cl !== ws);
    clientByUserId = clientByUserId.map((x) => (x === ws ? undefined : x));
  });

  svr.on('connection', (ws) => {
    clients.push(ws);
    console.log('New websocket connection');

    // runs a callback on message event
    ws.on('message', (data) => {
      console.log({ data });
      const dataObj = JSON.parse(data.toString());
      if (dataObj.type === 'token') {
        clients.find((v) => v === ws).token = dataObj.token;
      } else if (dataObj.type === 'uiEvent') {
        const token = clients.find((v) => v === ws).token;
        const authoruserId = tokenByUserId.findIndex((v) => v === token);
        const actorId = getUserActorOrZero(server, authoruserId);
        clientByUserId[authoruserId] = ws;
        onUiEvent(actorId, dataObj.msg);
      } else if (dataObj.type === 'chatMessage') {
        const token = clients.find((v) => v === ws).token;

        const authoruserId = tokenByUserId.findIndex((v) => v === token);

        const actorId = getUserActorOrZero(server, authoruserId);
        if (!actorId) return;
        dataObj.author = server.getActorName(actorId);

        const auhtorPos = server.getActorPos(actorId);
        const authorCellOrWorld = server.getActorCellOrWorld(actorId);

        const nonRp = 2;
        if (dataObj.channelIdx === nonRp && dataObj.text !== '') {
          dataObj.text = '(( ' + dataObj.text + ' ))';
        }

        svr.clients.forEach((client) => {
          if (client.readyState !== WebSocket.OPEN) return;

          const clientToken: string = clients.find((v) => v === client).token as string;

          const userId = tokenByUserId.findIndex((v) => v === clientToken);
          if (userId === -1) return;

          const id = getUserActorOrZero(server, userId);
          if (!id) return;

          const actorPos = server.getActorPos(id);
          const d = distance(actorPos, auhtorPos);

          if ((d >= 70 * 20 || server.getActorCellOrWorld(id) !== authorCellOrWorld) && dataObj.channelIdx !== nonRp)
            return;

          client.send(JSON.stringify(dataObj));
        });
      }
    });
  });
};
