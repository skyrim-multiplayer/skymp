import { ArgumentParser, ArgumentParserOptions } from 'argparse';
import * as fs from 'fs';

export class Settings {
	ip!: string;
	port = 7777;
	maxPlayers = 100;
	master!: string;
	name = 'Yet Another Server';
	gamemodePath = '...';
	loadOrder = new Array<string>();
	dataDir = './data';
	startPoints = [
		{ pos: [22659, -8697, -3594], worldOrCell: '0x1a26f', angleZ: 268 },
	];

	constructor() {
		if (fs.existsSync('./skymp5-gamemode')) {
			this.gamemodePath = './skymp5-gamemode/gamemode.js';
		} else {
			this.gamemodePath = './gamemode.js';
		}

		if (fs.existsSync('./server-settings.json')) {
			const parsed = JSON.parse(
				fs.readFileSync('./server-settings.json', 'utf-8')
			);
			[
				'ip',
				'port',
				'maxPlayers',
				'master',
				'name',
				'gamemodePath',
				'loadOrder',
				'dataDir',
				'startPoints',
			].forEach((prop) => {
				(this as Record<string, unknown>)[prop] = parsed[prop];
			});
		}
	}

	static get(): Settings {
		const args = Settings.parseArgs();
		const res = new Settings();

		res.port = +args['port'] || res.port;
		res.maxPlayers = +args['maxPlayers'] || res.maxPlayers;
		res.master = args['master'];
		res.name = args['name'] || res.name;
		res.ip = args['ip'] || res.ip;

		return res;
	}

	private static parseArgs() {
		const option: ArgumentParserOptions = {
			add_help: false,
			description: '',
		};
		const parser = new ArgumentParser(option);
		parser.add_argument('-m', '--maxPlayers', {});
		parser.add_argument('--master', {});
		parser.add_argument('--name', {});
		parser.add_argument('--port', {});
		parser.add_argument('--ip', {});
		return parser.parse_args();
	}
}
