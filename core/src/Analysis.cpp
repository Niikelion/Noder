#include <Noder/Analysis.hpp>
#include <unordered_map>

namespace Noder
{
	StaticAnalyser::Result StaticAnalyser::analyse(const Node& startPoint)
	{
		std::unordered_set<const Node*> visitedNodes;
		std::vector<const Node*> nodesToVisit{&startPoint};

		Result res;

		//traverse flow graph
		while (!nodesToVisit.empty())
		{
			const Node* node = nodesToVisit.back();
			nodesToVisit.pop_back();

			//ensure that this node was not visited before
			if (visitedNodes.count(node) == 0)
			{
				//mark as visited
				visitedNodes.emplace(node);
				//analyse value dependecny
				subAnalyse(*node, visitedNodes, res);
				//check if leaf
				if (node->getBase()->flowOutputPoints == 0)
				{
					//gather leaf node
					res.leafNodes.emplace_back(node);
				}
				else
				{
					//schedule nodes connected to flow outputs for processing
					for (size_t i = 0; i < node->getBase()->flowOutputPoints; ++i)
					{
						auto port = node->getFlowOutputPortTarget(i);
						//skip nonexisting connections
						if (!port.isVoid())
							nodesToVisit.emplace_back(&port.getNode());
					}
				}
			}
		}

		return res;
	}
	void StaticAnalyser::subAnalyse(const Node& startPoint, std::unordered_set<const Node*>& visitedNodes, Result& res)
	{
		std::vector<std::pair<const Node*, size_t>> nodesToVisit{ {&startPoint,0} };
		while (!nodesToVisit.empty())
		{
			const auto stackEntry = nodesToVisit.back();
			const Node* node = stackEntry.first;

			//check if dependencies are ready
			bool leap = false;
			std::vector<const Node*> depsToVisit;
			for (size_t i = 0; i < node->getBase()->inputs.size(); ++i)
			{
				auto port = node->getInputPortTarget(i);

				if (port.isVoid())
				{
					//missing input
					res.incompleteNodes.emplace_back(node);
				}
				else
				{
					//check if dependency was visited
					const Node& target = port.getNode();
					if (visitedNodes.count(&target) == 0)
					{
						//TODO: add check for accessing nodes from future
						if (target.getBase()->isFlowNode())
						{
							if (!leap)
							{
								res.leaps.emplace_back(node);
								leap = true;
							}
						}
						else
							depsToVisit.emplace_back(&target);
					}
				}
			}

			//check if already visited and not resolved
			if (visitedNodes.count(node) > 0 && !depsToVisit.empty())
			{
				//cycle detected
				nodesToVisit.pop_back();
				
				//reconstruct cycle
				res.cycles.emplace_back();
				size_t i = stackEntry.second;
				while (i > 0 && nodesToVisit[i].first != node)
				{
					res.cycles.back().emplace_back(nodesToVisit[i].first);
					i = nodesToVisit[i].second;
				}
				res.cycles.back().emplace_back(nodesToVisit[i].first);
				std::reverse(res.cycles.back().begin(), res.cycles.back().end());
			}
			else
			{
				if (depsToVisit.empty() || leap)
				{
					nodesToVisit.pop_back();
				}
				else
				{
					size_t s = nodesToVisit.size() - 1;
					for (auto dep : depsToVisit)
						nodesToVisit.emplace_back(dep, s);
				}
				//mark as visited
				visitedNodes.emplace(node);
			}
		}
	}
}