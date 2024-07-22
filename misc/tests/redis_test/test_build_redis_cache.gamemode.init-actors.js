mp.createActor(0xff000000, [0, 0, 0], 0, 0x0000003c);
mp.createActor(0xff000001, [0, 0, 1000], 0, 0x0000003c);
mp.createActor(0xff000002, [0, 0, 2000], 0, 0x0000003c);

const interval = setInterval(() => {
    if (!mp.isDatabaseBusy()) {
        console.log("gm finished");
        clearInterval(interval);
        process.exit(0);
    }
}, 1000);