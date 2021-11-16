#pragma once

#include <Noder/Nodes.hpp>
#include <unordered_set>

namespace Noder
{
	//TODO: add detecting usage paths for pure value nodes //possibly store only common part
	//that will help with code reordering for compiler to
	//allow generating constant calculation code once per function
	//instead of once per usage
	//TODO: add mapping flow node->list value node indicating
	//that after that node we should calculate given list of nodes
	//value node needs to be calculated right after node before the last node of common part of all usage paths
	class StaticAnalyser
	{
	public:
		struct Result
		{
			//nodes reachable from start point in flow graph, without any outgoing flow connections 
			std::vector<const Node*> leafNodes;
			//nodes with missing value inputs
			std::vector<const Node*> incompleteNodes;
			//disjoint cycles
			std::vector<std::vector<const Node*>> cycles;
			//attempts to leap into the future
			std::vector<const Node*> leaps;
		};

		static Result analyse(const Node& startPoint);
	private:
		static void subAnalyse(const Node& startPoint, std::unordered_set<const Node*>& visitedNodes, Result& res);
	};
}