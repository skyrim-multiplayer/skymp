The client sends a lot of packets to the server so that the game can function. In this tutorial, we'll look at how to create a new packet type and teach the server how to process it.

0. We need to figure out how the package will be called and what data will be in it. In [MsgType.h](https://github.com/skyrim-multiplayer/skymp/blob/main/skymp5-server/cpp/mp_common/MsgType.h) you can see how our names usually look. Our messages are mostly in JSON format. You can read the JSON syntax on Wikipedia https://en.wikipedia.org/wiki/JSON to remind yourself what types of data can be there in principle.
1. In `MsgType.h` add a new package type to enum (let it be Foo)
2. In `PacketParser.cpp`, `TransformPacketIntoAction`, add a case for our new package type: `case MsgType::Foo:`. Here, by analogy with neighboring cases, we get data from a JSON message into variables like `uint32_t`, `const char *`, etc.
3. In [ActionListener.h](https://github.com/skyrim-multiplayer/skymp/blob/main/skymp5-server/cpp/server_guest_lib/ActionListener.h) and [ActionListener.cpp](https://github.com/skyrim-multiplayer/skymp/blob/main/skymp5-server/cpp/server_guest_lib/ActionListener.cpp) add a method that will be called with the received data from the JSON message. In `ActionListener.cpp` we will implement it in the next step, but for now we will leave it empty. Now in `PacketParser.cpp`, in the newly created case, let's add a call to our new method. Voila, the server will call the method when receiving the packet.
4. The most interesting: we want to implement the method in `ActionListener.cpp`. Here you need to write code at any cost that will solve the problem. You can use various classes that are available in the server. But I advise you to adhere to a number of general recommendations written below.

General recommendations

1. Do not trust the data from the package. Be sure to check if the player can actually perform such an action. We will assume that we will have evil programmer players with access to the client code, especially since this is the case.
2. Recommendations will be supplemented
