'use strict';

const noopSpan = {
  spanContext() {
    return {
      traceId: '',
      spanId: '',
    };
  },
};

module.exports = {
  context: {
    active() {
      return null;
    },
  },
  trace: {
    getSpan() {
      return null;
    },
    getTracer() {
      return noopSpan;
    },
  },
};
