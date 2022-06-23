The packets we send from the client have an integer field `t` indicating the type of the packet.

1. In `messages.ts` there is an enum `MsgType`, let's add our package type there.
2. Now we send something in the `skympClient.ts` file from the client (usually). And `this.sendTarget.send` is used for this. This file is quite large, and a lot of things are sent from there, so we do Ctrl + F and drive in "MsgType" to see examples. I advise you to focus on how MsgType.Activate is sent.
