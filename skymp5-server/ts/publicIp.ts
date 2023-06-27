import Axios from 'axios';

export const getMyPublicIp = async (): Promise<string> => {
  const res = await Axios.request({
    url: 'http://api.ipify.org',
  });
  return res.data;
};
