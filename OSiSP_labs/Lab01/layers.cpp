#include <windows.h>
#include <vector>
#include "shape.cpp"

class Layers
{
private:
	std::vector<std::vector<Shape*>*>* layers;
	int layersCount = 0;
	std::vector<Shape*>* currentLayer = nullptr;
public:
	Layers()
	{
		layers = new std::vector<std::vector<Shape*>*>;
		addLayer();
	}
	int getLayersCount()
	{
		return layersCount;
	}
	std::vector<Shape*>* getCurrentLayer()
	{
		return currentLayer;
	}
	std::vector<Shape*>* addLayer()
	{
		layers->push_back(new std::vector<Shape*>);
		currentLayer = (*layers)[layersCount++];
	}
	std::vector<Shape*>* operator[](int index)
	{
		return (*layers)[index];
	}
	std::vector<Shape*>* setCurrentLayer(int index)
	{
		if (getLayersCount() > index && index >= 0)
			currentLayer = (*layers)[index];
		return currentLayer;
	}
	~Layers()
	{
		for (int i = 0; i < layers->size(); i++)
		{
			for (int j = 0; j < layers[i].size(); j++)
			{
				delete(layers[i][j]);
			}
			layers[i].clear();
		}
		layers->clear();
	}
};
