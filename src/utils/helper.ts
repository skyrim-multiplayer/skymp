/**
 *	check if array is equal
 * @param arr1 first array
 * @param arr2 second array
 */
export const isArrayEqual = (arr1: any, arr2: any): boolean => {
  const type = Object.prototype.toString.call(arr1);

  if (type !== Object.prototype.toString.call(arr2)) return false;

  if (['[object Array]', '[object Object]'].indexOf(type) < 0) return false;

  const valueLen = type === '[object Array]' ? arr1.length : Object.keys(arr1).length;
  const otherLen = type === '[object Array]' ? arr2.length : Object.keys(arr2).length;

  if (valueLen !== otherLen) return false;

  const compare = (item1: any, item2: any) => {
    const itemType = Object.prototype.toString.call(item1);
    if (['[object Array]', '[object Object]'].indexOf(itemType) >= 0) {
      if (!isArrayEqual(item1, item2)) return false;
    } else {
      if (itemType !== Object.prototype.toString.call(item2)) return false;
      if (itemType === '[object Function]') {
        if (item1.toString() !== item2.toString()) return false;
      } else {
        if (item1 !== item2) return false;
      }
    }
  };
  if (type === '[object Array]') {
    for (var i = 0; i < valueLen; i++) {
      if (compare(arr1[i], arr2[i]) === false) return false;
    }
  } else {
    for (var key in arr1) {
      if (arr1.hasOwnProperty(key)) {
        if (compare(arr1[key], arr2[key]) === false) return false;
      }
    }
  }

  return true;
};
