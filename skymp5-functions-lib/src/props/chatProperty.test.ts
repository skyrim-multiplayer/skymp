import { createSystemMessage } from '../props/chatProperty';

describe('chatProperty', () => {
  test('createSystemMessage', () => {
    const message = createSystemMessage('This should be sent by system');
    expect(message).toEqual({
      sender: { gameId: '0', masterApiId: 0 },
      category: 'system',
      opacity: 1,
      text: [{ color: '#FFFFFF', text: 'This should be sent by system' }],
    });
  });
});
