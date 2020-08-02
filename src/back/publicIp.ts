import Axios from "axios";

export const getMyPublicIp = async (): Promise<string> => {
  const res = await Axios.request({
    url: "http://ipv4bot.whatismyipaddress.com",
  });
  return res.data;
};
