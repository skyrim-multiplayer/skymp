#pragma once

#include <vector>
#include <algorithm>

class Utility
{
public:
	Utility(void);
	~Utility(void);

	template<typename T>
	static bool ContainsVector(std::vector<std::vector<T>> vectorList, std::vector<T> vectorItem)
	{		
		sort(vectorItem.begin(), vectorItem.end());
        for (int i = 0; i < vectorList.size(); i++)
        {
            std::vector<T> temp = vectorList[i];
            if (temp.size() == vectorItem.size())
            {                    
				sort(temp.begin(), temp.end());					
				if (equal(temp.begin(), temp.end(), vectorItem.begin()))                    
                {
                    return true;
                }
            }
        }
        return false;
	}

	template<typename T>
	static void FreeVectorMemory(std::vector<T> &obj)
	{
		obj.clear();
		std::vector<T>().swap(obj);
	}

	template<typename T>
	static void FreeVectorListMemory(std::vector<std::vector<T>> &objList)
	{
		for(int i=0; i<objList.size(); i++)
		{
			objList[i].clear();
			std::vector<T>().swap(objList[i]);
		}

		objList.clear();
		std::vector<std::vector<T>>().swap(objList);
	}
};

