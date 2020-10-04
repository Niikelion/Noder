#include "noder/Interpreter.hpp"
#include <stdexcept>

namespace Noder
{
	void NodeInterpreter::NodeState::resetInternals()
	{
		targetFlowPort = 0;
		pushesScope = false;
	}
	void NodeInterpreter::NodeState::softReset()
	{
		resetInternals();
		for (auto& output : cachedOutputs)
		{
			if (output)
				output.get() -> reset();
		}
	}

	void NodeInterpreter::NodeState::hardReset()
	{
		softReset();
		resetState();
	}

	void NodeInterpreter::NodeState::setUpInternals(std::unique_ptr<State>& scopeVariable)
	{
		scopedVariable = &scopeVariable;
	}

	void NodeInterpreter::NodeState::attachNode(const Node& node)
	{
		auto& outputs = node.getBase()->outputs;
		cachedOutputs.clear();
		cachedOutputs.reserve(outputs.size());
		for (const auto& port : outputs)
		{
			cachedOutputs.emplace_back(port.createState());
		}
	}

	std::string NodeInterpreter::correctName(const std::string& name)
	{
		return (name.length() == 0) ? ("__generated" + std::to_string(stateFactories.size())) : name;
	}

	void NodeInterpreter::buildState(Node& node)
	{
		auto factory = stateFactories.find(node.getBase()->action);
		if (factory != stateFactories.end())
		{
			auto it = states.emplace(std::piecewise_construct, std::forward_as_tuple(&node), std::forward_as_tuple(factory->second(node,scopedVariable)));
		}
	}

	void NodeInterpreter::rebuildState(Node& node)
	{
		if (states.count(&node))
			states.erase(&node);
		buildState(node);
	}

	void NodeInterpreter::buildStates()
	{
		states.clear();
		for (auto& i : env->nodes)
		{
			buildState(*i);
		}
	}

	Node& NodeInterpreter::createNode(NodeTemplate& t)
	{
		Node& n = env->createNode(t);
		buildState(n);
		return n;
	}
	NodeTemplate& NodeInterpreter::createTemplate()
	{
		return env->createTemplate();
	}
	
	NodeTemplate& NodeInterpreter::createTemplate(const std::string& name, const std::vector<Port>& inP, const std::vector<Port>& outP, unsigned flowInP, unsigned flowOutP, const std::function<std::unique_ptr<NodeState>(const Node&, std::unique_ptr<State>&)>& factory)
	{
		NodeTemplate& t = env->createTemplate(name, inP, outP, flowInP, flowOutP);
		addStateFactory(name, factory);

		return t;
	}
	
	void NodeInterpreter::addStateFactory(const std::string& name ,const std::function<std::unique_ptr<NodeState>(const Node&, std::unique_ptr<State>&)>& factory)
	{
		if (!stateFactories.count(name))
			stateFactories.emplace(name, factory);
	}

	NodeInterpreter::NodeState* NodeInterpreter::getNodeState(Node& n)
	{
		auto it = states.find(&n);
		if (it == states.end())
		{
			throw std::invalid_argument("Node does not exist in current enviroment.");
		}
		return it->second.get();
	}

	std::unique_ptr<Enviroment> NodeInterpreter::extractEnviroment()
	{
		states.clear();
		std::unique_ptr<Enviroment> cpy = std::move(env);
		env = std::make_unique<Enviroment>();
		return cpy;
	}
	std::unique_ptr<Enviroment> NodeInterpreter::swapEnviroment(std::unique_ptr<Enviroment>&& t)
	{
		std::unique_ptr<Enviroment> tmp = std::move(env);
		env = std::move(t);
		states.clear();
		buildStates();
		return tmp;
	}
	Enviroment& NodeInterpreter::getEnviroment()
	{
		return *env;
	}
	void NodeInterpreter::resetEnviroment()
	{
		states.clear();
		env->clear();
	}
	void NodeInterpreter::resetStates()
	{
		for (auto& i : states)
		{
			i.second->hardReset();
		}
	}

	void NodeInterpreter::resetState(Node* node)
	{
		states.at(node)->hardReset();
	}

	void NodeInterpreter::softResetState(Node* node)
	{
		states.at(node)->softReset();
	}

	void NodeInterpreter::resetFactories()
	{
		stateFactories.clear();
	}

	void NodeInterpreter::runFrom(Node& startPoint)
	{
		std::unordered_set<Node*>tmp, toReset;
		Node* c = &startPoint;

		unsigned redirected = 0;

		while (c != nullptr)
		{
			NodeState& state = *states.at(c);

			state.resetInternals();

			calcNode(*c, tmp, toReset);

			if (state.pushesNewScope())
				pushScope(c);

			redirected = state.getTargetFlowPort();

			Node::PortWrapper portOfNext = c->getFlowOutputPortTarget(redirected);
			//exit if port is not connected
			if (portOfNext.isVoid())
			{
				//pop scope and return if already in main scope
				c = popScope();
			}
			else
			{
				//get next node
				c = const_cast<Node*>(portOfNext.getNode());
			}
		}
		resetStates();
	}
	void NodeInterpreter::calcNode(Node& endPoint)
	{
		std::unordered_set<Node*>tmp, toReset;
		calcNode(endPoint, tmp, toReset);
	}

	void NodeInterpreter::pushScope(Node* n)
	{
		pushedScopes.emplace_back(n);
	}

	Node* NodeInterpreter::popScope()
	{
		if (pushedScopes.size() > 0)
		{
			Node* r = pushedScopes.back().origin;

			for (auto node : pushedScopes.back().nodesToReset)
			{
				resetState(node);
			}

			pushedScopes.pop_back();
			return r;
		}
		return nullptr;
	}

	void NodeInterpreter::calcNode(Node& endPoint, std::unordered_set<Node*>& visited, std::unordered_set<Node*>& toReset)
	{
		//TODO: convert recursion to iteration
		if (visited.find(&endPoint) != visited.end())
		{
			throw NodeCycleException();
		}
		visited.insert(&endPoint);

		std::vector<const State*> inputs;
		inputs.resize(endPoint.getBase()->inputs.size());

		//convert to iterative approach
		for (unsigned i = 0; i < inputs.size(); ++i)
		{
			Node::PortWrapper p = endPoint.getInputPortTarget(i);
			if (p.isVoid())
			{
				const PortState* state = endPoint.getInputState(i);

				if (state != nullptr && state->isReady())
				{
					inputs[i] = state;
				}
				else
				{
					throw std::logic_error("Port not connected and initialized.");
				}
			}
			else
			{
				auto it = states.find(p.getNode());
				if (it != states.end())
				{
					State* state = it->second->getCachedOutputs()[p.getPort()].get();
					if (!state->isReady())
					{
						if (p.getNode()->usedFlowInputs() > 0)
						{
							throw std::logic_error("Cannot access node from the future.");
						}
						calcNode(*const_cast<Node*>(p.getNode()), visited, toReset);
					}
					inputs[i] = state;
				}
				else
				{
					throw std::logic_error("Illegal state found.");
				}
			}
		}
		auto it = states.find(&endPoint);
		if (it != states.end())
		{
			it->second->calculate(inputs, it->second->getCachedOutputs());
		}
		if (endPoint.usedFlowInputs() > 0 && endPoint.getBase()->flowInputPoints == endPoint.usedFlowInputs())
		{
			toReset.insert(&endPoint);
		}
	}

	NodeInterpreter::NodeInterpreter()
	{
		env = std::make_unique<Enviroment>();
	}
	NodeInterpreter::NodeInterpreter(std::unique_ptr<Enviroment>&& t)
	{
		env = std::move(t);
		buildStates();
	}
}