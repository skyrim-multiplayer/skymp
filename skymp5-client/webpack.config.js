//webpack.config.js
const path = require('path');
//const WebpackSystemRegister = require('webpack-system-register');

module.exports = {
    mode: "development",
    //mode: "production",
    // entry: {
    //     main: "./src/index.ts",
    // },
    entry: [
        //"./node_modules/core-js/shim",
        "./src/index.ts"
    ],
    // plugins: [
    //     new WebpackSystemRegister({}),
    // ],
    devtool: "inline-source-map",
    output: {
        libraryTarget: "system",
        path: path.resolve(__dirname, '../build/dist/server/dist_front'),
        filename: "skymp5-client.js", // <--- Will be compiled to this single file
    },
    resolve: {
        modules: [
            path.resolve(__dirname, '../build/dist/client/Data/Platform/Modules'),
            "node_modules"
        ],
        extensions: [".ts", ".tsx", ".js"],
    },
    optimization: {
        minimize: false
    },
    module: {
        rules: [
            // {
            //     test: /\.tsx?$/,
            //     include: [
            //         path.resolve(__dirname, '../build/dist/client/Data/Platform/Modules')
            //     ],
            //     use: "ts-loader"
            // },
            {
                test: /\.tsx?$/,
                // include: [
                //     path.resolve(__dirname, 'src'),
                //     path.resolve(__dirname, '../build/dist/client/Data/Platform/Modules')
                // ],
                use: "ts-loader"
            }
        ]
    },
    // externals: {
    //     "skyrimPlatform": path.resolve(__dirname, "../build/dist/client/Data/Platform/Modules/skyrimPlatform.ts")
    // }
};