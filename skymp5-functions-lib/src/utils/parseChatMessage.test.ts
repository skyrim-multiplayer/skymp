import { parseChatMessage } from './parseChatMessage';

describe('parseChatMessage', () => {
  test('All included', () => {
    const message = '%whisper *action* ((nonrp)) more text% №shout№';
    const expected = [
      { text: 'whisper ', color: '#A062C9', type: ['whisper'] },
      { text: 'action', color: '#CFAA6E', type: ['whisper', 'action'] },
      { text: ' ', color: '#A062C9', type: ['whisper'] },
      { text: '((nonrp))', color: '#91916D', type: ['whisper', 'nonrp'] },
      { text: ' more text', color: '#A062C9', type: ['whisper'] },
      { text: ' ', color: '#FFFFFF', type: ['plain'] },
      { text: 'shout', color: '#F78C8C', type: ['shout'] },
    ];
    expect(parseChatMessage(message)).toStrictEqual(expected);
  });
  test('Plain message', () => {
    const message = "Hello! I'm on server!";
    const expected = [{ text: "Hello! I'm on server!", color: '#FFFFFF', type: ['plain'] }];
    expect(parseChatMessage(message)).toStrictEqual(expected);
  });
  test('Nonrp', () => {
    const message = "((Hello! I'm on server!))";
    const expected = [{ text: "((Hello! I'm on server!))", color: '#91916D', type: ['nonrp'] }];
    expect(parseChatMessage(message)).toStrictEqual(expected);
  });
  test('Action', () => {
    const message = "*Hello! I'm on server!*";
    const expected = [{ text: "Hello! I'm on server!", color: '#CFAA6E', type: ['action'] }];
    expect(parseChatMessage(message)).toStrictEqual(expected);
  });
  test("Shout can't be nested", () => {
    const message = '((nonrp №shout№))';
    const expected = [{ text: '((nonrp shout))', color: '#91916D', type: ['nonrp'] }];
    expect(parseChatMessage(message)).toStrictEqual(expected);
  });
  test('Whisper', () => {
    const message = "%Hello! I'm on server!%";
    const expected = [{ text: "Hello! I'm on server!", color: '#A062C9', type: ['whisper'] }];
    expect(parseChatMessage(message)).toStrictEqual(expected);
  });
  test('Nesting', () => {
    const message = '((nonrp *action*))';
    const expected = [
      { text: '((nonrp ', color: '#91916D', type: ['nonrp'] },
      { text: 'action))', color: '#CFAA6E', type: ['nonrp', 'action'] },
    ];
    expect(parseChatMessage(message)).toStrictEqual(expected);
  });
});
