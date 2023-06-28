export const replaceIfMoreThan20 = (str, substring, newString, max) => {
  const regex = new RegExp(substring, 'g'); // create a regular expression to match all occurrences of substring
  const count = (str.match(regex) || []).length; // count the number of occurrences of substring in str

  if (count > max) { // check if the count is more than max
    const replaceCount = count - max; // calculate the number of occurrences to replace
    let replacedCount = 0; // initialize the counter for replaced occurrences

    // create an array of all the occurrences of the substring
    const occurrences = Array.from(str.matchAll(regex)).map((match) => match.index);

    // sort the array of occurrences in descending order
    occurrences.sort((a, b) => b - a);

    // replace the occurrences that exceed max, starting from the end
    let result = str;
    for (const index of occurrences) {
      if (replacedCount >= replaceCount) break; // stop if the number of replaced occurrences equals replaceCount
      result = result.substring(0, index) + newString + result.substring(index + substring.length);
      replacedCount++;
    }
    return result;
  } else {
    return str; // return the original string if the count is not more than max
  }
};
