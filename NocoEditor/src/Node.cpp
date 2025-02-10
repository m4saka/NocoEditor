﻿#include "NocoUI/Node.hpp"
#include "NocoUI/Canvas.hpp"
#include "NocoUI/Component/Component.hpp"

namespace noco
{
	InteractState Node::updateForCurrentInteractState(const std::shared_ptr<Node>& hoveredNode, InteractableYN parentInteractable)
	{
		const InteractableYN interactable{ m_interactable && parentInteractable };
		InteractState inheritedInteractState = InteractState::Default;
		bool inheritedIsClicked = false;
		if (m_inheritChildrenStateFlags != InheritChildrenStateFlags::None)
		{
			for (const auto& child : m_children)
			{
				InteractState childInteractState = child->updateForCurrentInteractState(hoveredNode, interactable);
				if (interactable)
				{
					if (!inheritsChildrenPressedState() && childInteractState == InteractState::Pressed)
					{
						childInteractState = InteractState::Hovered;
					}
					if (!inheritsChildrenHoveredState() && childInteractState == InteractState::Hovered)
					{
						childInteractState = InteractState::Default;
					}
					if (inheritsChildrenPressedState() && child->isClicked())
					{
						inheritedIsClicked = true;
					}
					inheritedInteractState = ApplyOtherInteractState(inheritedInteractState, childInteractState, AppliesDisabledStateYN::No);
				}
			}
		}
		if (interactable)
		{
			const bool mouseOverForHovered = m_activeInHierarchy && (hoveredNode.get() == this || (inheritsChildrenHoveredState() && (inheritedInteractState == InteractState::Hovered || inheritedInteractState == InteractState::Pressed)));
			const bool mouseOverForPressed = m_activeInHierarchy && (hoveredNode.get() == this || (inheritsChildrenPressedState() && (inheritedInteractState == InteractState::Pressed || inheritedIsClicked))); // クリック判定用に離した瞬間もホバー扱いにする必要があるため、子のisClickedも加味している
			m_mouseLTracker.update(mouseOverForHovered, mouseOverForPressed);
			return ApplyOtherInteractState(m_mouseLTracker.interactStateSelf(), inheritedInteractState);
		}
		else
		{
			m_mouseLTracker.update(false, false);
			return InteractState::Disabled;
		}
	}

	InteractState Node::updateForCurrentInteractStateRight(const std::shared_ptr<Node>& hoveredNode, InteractableYN parentInteractable)
	{
		const InteractableYN interactable{ m_interactable && parentInteractable };
		InteractState inheritedInteractState = InteractState::Default;
		bool inheritedIsRightClicked = false;
		if (m_inheritChildrenStateFlags != InheritChildrenStateFlags::None)
		{
			for (const auto& child : m_children)
			{
				InteractState childInteractState = child->updateForCurrentInteractStateRight(hoveredNode, interactable);
				if (interactable)
				{
					if (!inheritsChildrenPressedState() && childInteractState == InteractState::Pressed)
					{
						childInteractState = InteractState::Hovered;
					}
					if (!inheritsChildrenHoveredState() && childInteractState == InteractState::Hovered)
					{
						childInteractState = InteractState::Default;
					}
					if (inheritsChildrenPressedState() && child->isRightClicked())
					{
						inheritedIsRightClicked = true;
					}
					inheritedInteractState = ApplyOtherInteractState(inheritedInteractState, childInteractState, AppliesDisabledStateYN::No);
				}
			}
		}
		if (interactable)
		{
			const bool mouseOverForHovered = m_activeInHierarchy && (hoveredNode.get() == this || (inheritsChildrenHoveredState() && (inheritedInteractState == InteractState::Hovered || inheritedInteractState == InteractState::Pressed)));
			const bool mouseOverForPressed = m_activeInHierarchy && (hoveredNode.get() == this || (inheritsChildrenPressedState() && (inheritedInteractState == InteractState::Pressed || inheritedIsRightClicked))); // クリック判定用に離した瞬間もホバー扱いにする必要があるため、子のisRightClickedも加味している
			m_mouseRTracker.update(mouseOverForHovered, mouseOverForPressed);
			return ApplyOtherInteractState(m_mouseRTracker.interactStateSelf(), inheritedInteractState);
		}
		else
		{
			m_mouseRTracker.update(false, false);
			return InteractState::Disabled;
		}
	}

	void Node::refreshActiveInHierarchy()
	{
		m_activeInHierarchy = ActiveYN{ m_activeSelf && (m_parent.expired() || m_parent.lock()->m_activeInHierarchy) };
		for (const auto& child : m_children)
		{
			child->refreshActiveInHierarchy();
		}
	}

	void Node::setCanvasRecursive(const std::weak_ptr<Canvas>& canvas)
	{
		m_canvas = canvas;
		for (const auto& child : m_children)
		{
			child->setCanvasRecursive(canvas);
		}
	}

	void Node::clampScrollOffset()
	{
		if (m_scrollOffset == Vec2::Zero())
		{
			return;
		}

		const bool horizontalScrollableValue = horizontalScrollable();
		const bool verticalScrollableValue = verticalScrollable();
		if (!horizontalScrollableValue && !verticalScrollableValue)
		{
			m_scrollOffset = Vec2::Zero();
			return;
		}

		if (!horizontalScrollableValue)
		{
			m_scrollOffset.x = 0.0;
		}
		if (!verticalScrollableValue)
		{
			m_scrollOffset.y = 0.0;
		}

		const Optional<RectF> contentRectOpt = getChildrenContentRectWithPadding();
		if (!contentRectOpt)
		{
			m_scrollOffset = Vec2::Zero();
			return;
		}

		const RectF& contentRect = *contentRectOpt;

		const double viewWidth = m_layoutAppliedRect.w;
		const double viewHeight = m_layoutAppliedRect.h;

		const double maxScrollX = contentRect.w > viewWidth
			? contentRect.w - viewWidth
			: 0.0;

		const double maxScrollY = contentRect.h > viewHeight
			? contentRect.h - viewHeight
			: 0.0;

		m_scrollOffset.x = Clamp(m_scrollOffset.x, 0.0, maxScrollX);
		m_scrollOffset.y = Clamp(m_scrollOffset.y, 0.0, maxScrollY);
	}

	std::shared_ptr<Node> Node::Create(StringView name, const ConstraintVariant& constraint, IsHitTargetYN isHitTarget, InheritChildrenStateFlags inheritChildrenStateFlags)
	{
		return std::shared_ptr<Node>{ new Node{ name, constraint, isHitTarget, inheritChildrenStateFlags } };
	}

	const ConstraintVariant& Node::constraint() const
	{
		return m_constraint;
	}

	void Node::setConstraint(const ConstraintVariant& constraint, RefreshesLayoutYN refreshesLayout)
	{
		m_constraint = constraint;
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
	}

	const BoxConstraint* Node::boxConstraint() const
	{
		return std::get_if<BoxConstraint>(&m_constraint);
	}

	const AnchorConstraint* Node::anchorConstraint() const
	{
		return std::get_if<AnchorConstraint>(&m_constraint);
	}

	TransformEffect& Node::transformEffect()
	{
		return m_transformEffect;
	}

	const TransformEffect& Node::transformEffect() const
	{
		return m_transformEffect;
	}

	const LayoutVariant& Node::layout() const
	{
		return m_layout;
	}

	void Node::setLayout(const LayoutVariant& layout, RefreshesLayoutYN refreshesLayout)
	{
		m_layout = layout;
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
	}

	const FlowLayout* Node::flowLayout() const
	{
		return std::get_if<FlowLayout>(&m_layout);
	}

	const HorizontalLayout* Node::horizontalLayout() const
	{
		return std::get_if<HorizontalLayout>(&m_layout);
	}

	const VerticalLayout* Node::verticalLayout() const
	{
		return std::get_if<VerticalLayout>(&m_layout);
	}

	SizeF Node::fittingSizeToChildren() const
	{
		return std::visit([this](const auto& layout) { return layout.fittingSizeToChildren(m_layoutAppliedRect, m_children); }, m_layout);
	}

	void Node::setBoxConstraintToFitToChildren(FitTarget fitTarget, RefreshesLayoutYN refreshesLayout)
	{
		std::visit([this, fitTarget, refreshesLayout](auto& layout) { layout.setBoxConstraintToFitToChildren(m_layoutAppliedRect, m_children, *this, fitTarget, refreshesLayout); }, m_layout);
	}

	const LRTB& Node::layoutPadding() const
	{
		return std::visit([](const auto& layout) -> const LRTB& { return layout.padding; }, m_layout);
	}

	bool Node::isParentLayoutAffected() const
	{
		// AnchorConstraintの場合のみ親のレイアウトの影響を受けない
		return !std::holds_alternative<AnchorConstraint>(m_constraint);
	}

	JSON Node::toJSON() const
	{
		Array<JSON> childrenJSON;
		for (const auto& child : m_children)
		{
			childrenJSON.push_back(child->toJSON());
		}

		JSON result
		{
			{ U"name", m_name },
			{ U"constraint", std::visit([](const auto& constraint) { return constraint.toJSON(); }, m_constraint) },
			{ U"transformEffect", m_transformEffect.toJSON() },
			{ U"layout", std::visit([](const auto& layout) { return layout.toJSON(); }, m_layout) },
			{ U"components", Array<JSON>{} },
			{ U"children", childrenJSON },
			{ U"isHitTarget", m_isHitTarget.getBool() },
			{ U"inheritsChildrenHoveredState", inheritsChildrenHoveredState() },
			{ U"inheritsChildrenPressedState", inheritsChildrenPressedState() },
			{ U"interactable", m_interactable.getBool() },
			{ U"horizontalScrollable", horizontalScrollable() },
			{ U"verticalScrollable", verticalScrollable() },
			{ U"clippingEnabled", m_clippingEnabled.getBool() },
			{ U"activeSelf", m_activeSelf.getBool() },
		};

		for (const auto& component : m_components)
		{
			auto json = component->toJSON();
			if (!json.isNull())
			{
				result[U"components"].push_back(json);
			}
		}

		return result;
	}

	std::shared_ptr<Node> Node::FromJSON(const JSON& json)
	{
		auto node = Node::Create();
		if (json.contains(U"name"))
		{
			node->m_name = json[U"name"].getOr<String>(U"");
		}
		if (json.contains(U"constraint") && json[U"constraint"].contains(U"type"))
		{
			const auto type = json[U"constraint"][U"type"].getString();
			if (type == U"AnchorConstraint")
			{
				node->m_constraint = AnchorConstraint::FromJSON(json[U"constraint"]);
			}
			else if (type == U"BoxConstraint")
			{
				node->m_constraint = BoxConstraint::FromJSON(json[U"constraint"]);
			}
			else
			{
				// 不明な場合はBoxConstraint扱いにする
				// TODO: 不明な場合は警告を出力
				node->m_constraint = BoxConstraint{};
			}
		}
		if (json.contains(U"transformEffect"))
		{
			node->m_transformEffect.readFromJSON(json[U"transformEffect"]);
		}
		if (json.contains(U"layout") && json[U"layout"].contains(U"type"))
		{
			const auto type = json[U"layout"][U"type"].getString();
			if (type == U"FlowLayout")
			{
				node->m_layout = FlowLayout::FromJSON(json[U"layout"]);
			}
			else if (type == U"HorizontalLayout")
			{
				node->m_layout = HorizontalLayout::FromJSON(json[U"layout"]);
			}
			else if (type == U"VerticalLayout")
			{
				node->m_layout = VerticalLayout::FromJSON(json[U"layout"]);
			}
			else
			{
				// 不明な場合はFlowLayout扱いにする
				// TODO: 不明な場合は警告を出力
				node->m_layout = FlowLayout{};
			}
		}
		if (json.contains(U"isHitTarget"))
		{
			node->m_isHitTarget = IsHitTargetYN{ json[U"isHitTarget"].getOr<bool>(true) };
		}
		if (json.contains(U"inheritsChildrenHoveredState"))
		{
			node->setInheritsChildrenHoveredState(json[U"inheritsChildrenHoveredState"].getOr<bool>(false));
		}
		if (json.contains(U"inheritsChildrenPressedState"))
		{
			node->setInheritsChildrenPressedState(json[U"inheritsChildrenPressedState"].getOr<bool>(false));
		}
		if (json.contains(U"interactable"))
		{
			node->setInteractable(InteractableYN{ json[U"interactable"].getOr<bool>(true) });
		}
		if (json.contains(U"horizontalScrollable"))
		{
			node->setHorizontalScrollable(json[U"horizontalScrollable"].getOr<bool>(false), RefreshesLayoutYN::No);
		}
		if (json.contains(U"verticalScrollable"))
		{
			node->setVerticalScrollable(json[U"verticalScrollable"].getOr<bool>(false), RefreshesLayoutYN::No);
		}
		if (json.contains(U"clippingEnabled"))
		{
			node->setClippingEnabled(ClippingEnabledYN{ json[U"clippingEnabled"].getOr<bool>(false) });
		}
		if (json.contains(U"activeSelf"))
		{
			node->setActive(ActiveYN{ json[U"activeSelf"].getOr<bool>(true) }, RefreshesLayoutYN::No);
		}

		if (json.contains(U"components") && json[U"components"].isArray())
		{
			for (const auto& componentJSON : json[U"components"].arrayView())
			{
				const auto type = componentJSON[U"type"].getString();

				if (type == U"Label")
				{
					auto label = std::make_shared<Label>();
					if (!label->tryReadFromJSON(componentJSON))
					{
						throw Error{ U"Failed to read Label component from JSON" };
					}
					node->addComponent(std::move(label));
					continue;
				}

				if (type == U"Sprite")
				{
					auto sprite = std::make_shared<Sprite>();
					if (!sprite->tryReadFromJSON(componentJSON))
					{
						throw Error{ U"Failed to read Sprite component from JSON" };
					}
					node->addComponent(std::move(sprite));
					continue;
				}

				if (type == U"RectRenderer")
				{
					auto rectRenderer = std::make_shared<RectRenderer>();
					if (!rectRenderer->tryReadFromJSON(componentJSON))
					{
						throw Error{ U"Failed to read RectRenderer component from JSON" };
					}
					node->addComponent(std::move(rectRenderer));
					continue;
				}

				if (type == U"TextBox")
				{
					auto textBox = std::make_shared<TextBox>();
					if (!textBox->tryReadFromJSON(componentJSON))
					{
						throw Error{ U"Failed to read TextBox component from JSON" };
					}
					node->addComponent(std::move(textBox));
					continue;
				}

				// TODO: 不明なコンポーネントの場合は警告を出力
			}
		}

		if (json.contains(U"children") && json[U"children"].isArray())
		{
			for (const auto& childJSON : json[U"children"].arrayView())
			{
				node->addChildFromJSON(childJSON, RefreshesLayoutYN::No);
			}
		}
		return node;
	}

	std::shared_ptr<Node> Node::parent() const
	{
		return m_parent.lock();
	}

	bool Node::removeFromParent()
	{
		if (const auto parent = m_parent.lock())
		{
			parent->removeChild(shared_from_this());
			return true;
		}
		return false;
	}

	void Node::addComponent(std::shared_ptr<ComponentBase>&& component)
	{
		if (m_componentsIterGuard.isIterating())
		{
			throw Error{ U"addComponent: Cannot add component while iterating" };
		}
		m_components.push_back(std::move(component));
	}

	void Node::addComponent(const std::shared_ptr<ComponentBase>& component)
	{
		if (m_componentsIterGuard.isIterating())
		{
			throw Error{ U"addComponent: Cannot add component while iterating" };
		}
		m_components.push_back(component);
	}

	void Node::removeComponent(const std::shared_ptr<ComponentBase>& component)
	{
		if (m_componentsIterGuard.isIterating())
		{
			throw Error{ U"removeComponent: Cannot remove component while iterating" };
		}
		m_components.remove(component);
	}

	bool Node::moveComponentUp(const std::shared_ptr<ComponentBase>& component)
	{
		if (m_componentsIterGuard.isIterating())
		{
			throw Error{ U"moveComponentUp: Cannot move component while iterating" };
		}
		const auto it = std::find(m_components.begin(), m_components.end(), component);
		if (it == m_components.end())
		{
			// 見つからない
			return false;
		}
		if (it == m_components.begin())
		{
			// 一番上にあるため上移動は不可
			return false;
		}
		std::iter_swap(it, std::prev(it));
		return true;
	}

	bool Node::moveComponentDown(const std::shared_ptr<ComponentBase>& component)
	{
		if (m_componentsIterGuard.isIterating())
		{
			throw Error{ U"moveComponentDown: Cannot move component while iterating" };
		}
		const auto it = std::find(m_components.begin(), m_components.end(), component);
		if (it == m_components.end())
		{
			// 見つからない
			return false;
		}
		if (it == std::prev(m_components.end()))
		{
			// 一番下にあるため下移動は不可
			return false;
		}
		std::iter_swap(it, std::next(it));
		return true;
	}

	const std::shared_ptr<Node>& Node::addChild(std::shared_ptr<Node>&& child, RefreshesLayoutYN refreshesLayout)
	{
		if (m_childrenIterGuard.isIterating())
		{
			throw Error{ U"addChild: Cannot add child while iterating" };
		}
		if (!child->m_parent.expired())
		{
			throw Error{ U"addChild: Child node '{}' already has a parent"_fmt(child->m_name) };
		}
		child->setCanvasRecursive(m_canvas);
		child->m_parent = shared_from_this();
		child->refreshActiveInHierarchy();
		m_children.push_back(std::move(child));
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
		return m_children.back();
	}

	const std::shared_ptr<Node>& Node::addChild(const std::shared_ptr<Node>& child, RefreshesLayoutYN refreshesLayout)
	{
		if (m_childrenIterGuard.isIterating())
		{
			throw Error{ U"addChild: Cannot add child while iterating" };
		}
		if (!child->m_parent.expired())
		{
			throw Error{ U"addChild: Child node '{}' already has a parent"_fmt(child->m_name) };
		}
		child->setCanvasRecursive(m_canvas);
		child->m_parent = shared_from_this();
		child->refreshActiveInHierarchy();
		m_children.push_back(child);
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
		return m_children.back();
	}

	const std::shared_ptr<Node>& Node::emplaceChild(StringView name, const ConstraintVariant& constraint, IsHitTargetYN isHitTarget, InheritChildrenStateFlags inheritChildrenStateFlags, RefreshesLayoutYN refreshesLayout)
	{
		if (m_childrenIterGuard.isIterating())
		{
			throw Error{ U"emplaceChild: Cannot emplace child while iterating" };
		}
		auto child = Node::Create(name, constraint, isHitTarget, inheritChildrenStateFlags);
		child->setCanvasRecursive(m_canvas);
		child->m_parent = shared_from_this();
		child->refreshActiveInHierarchy();
		m_children.push_back(std::move(child));
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
		return m_children.back();
	}

	const std::shared_ptr<Node>& Node::addChildAtIndex(const std::shared_ptr<Node>& child, size_t index, RefreshesLayoutYN refreshesLayout)
	{
		if (m_childrenIterGuard.isIterating())
		{
			throw Error{ U"addChildAtIndex: Cannot add child while iterating" };
		}
		if (!child->m_parent.expired())
		{
			throw Error{ U"addChildAtIndex: Child node '{}' already has a parent"_fmt(child->m_name) };
		}
		if (index > m_children.size())
		{
			index = m_children.size();
		}

		child->setCanvasRecursive(m_canvas);
		child->m_parent = shared_from_this();
		child->refreshActiveInHierarchy();
		m_children.insert(m_children.begin() + index, child);

		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}

		return m_children[index];
	}

	void Node::removeChild(const std::shared_ptr<Node>& child, RefreshesLayoutYN refreshesLayout)
	{
		if (m_childrenIterGuard.isIterating())
		{
			throw Error{ U"removeChild: Cannot remove child while iterating" };
		}
		if (!m_children.contains(child))
		{
			throw Error{ U"removeChild: Child node '{}' not found in node '{}'"_fmt(child->m_name, m_name) };
		}
		child->setCanvasRecursive(std::weak_ptr<Canvas>{});
		child->m_parent.reset();
		child->refreshActiveInHierarchy();
		m_children.remove(child);
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
	}

	bool Node::containsChild(const std::shared_ptr<Node>& child, RecursiveYN recursive) const
	{
		if (m_children.contains(child))
		{
			return true;
		}
		if (recursive)
		{
			for (const auto& c : m_children)
			{
				if (c->containsChild(child, RecursiveYN::Yes))
				{
					return true;
				}
			}
		}
		return false;
	}

	bool Node::containsChildByName(StringView name, RecursiveYN recursive) const
	{
		for (const auto& child : m_children)
		{
			if (child->m_name == name)
			{
				return true;
			}
		}
		if (recursive)
		{
			for (const auto& child : m_children)
			{
				if (child->containsChildByName(name, RecursiveYN::Yes))
				{
					return true;
				}
			}
		}
		return false;
	}

	std::shared_ptr<Node> Node::getChildByName(StringView name, RecursiveYN recursive)
	{
		for (const auto& child : m_children)
		{
			if (child->m_name == name)
			{
				return child;
			}
		}
		if (recursive)
		{
			for (const auto& child : m_children)
			{
				// 例外を投げられると途中で終了してしまうため、getChildByNameではなくgetChildByNameOrNullを使う必要がある
				if (const auto found = child->getChildByNameOrNull(name, RecursiveYN::Yes))
				{
					return found;
				}
			}
		}
		throw Error{ U"Child node '{}' not found in node '{}'"_fmt(name, m_name) };
	}

	std::shared_ptr<Node> Node::getChildByNameOrNull(StringView name, RecursiveYN recursive)
	{
		for (const auto& child : m_children)
		{
			if (child->m_name == name)
			{
				return child;
			}
		}
		if (recursive)
		{
			for (const auto& child : m_children)
			{
				if (const auto found = child->getChildByNameOrNull(name, RecursiveYN::Yes))
				{
					return found;
				}
			}
		}
		return nullptr;
	}

	void Node::refreshChildrenLayout()
	{
		std::visit([this](const auto& layout)
			{
				layout.execute(m_layoutAppliedRect, m_children, [this](const std::shared_ptr<Node>& child, const RectF& rect)
					{
						child->m_layoutAppliedRect = rect;
						if (child->isParentLayoutAffected())
						{
							child->m_layoutAppliedRect.moveBy(-m_scrollOffset);
						}
					});
			}, m_layout);
		for (const auto& child : m_children)
		{
			child->refreshChildrenLayout();
		}

		// レイアウト更新後の状態でスクロールオフセットを制限し、変化があれば反映
		// (子ノードの大きさに変更があった場合にもスクロールオフセットの制限を更新する必要があるため)
		const Vec2 prevScrollOffset = m_scrollOffset;
		clampScrollOffset();
		if (m_scrollOffset != prevScrollOffset)
		{
			for (const auto& child : m_children)
			{
				if (child->isParentLayoutAffected())
				{
					child->m_layoutAppliedRect.moveBy(prevScrollOffset - m_scrollOffset);
				}
			}
		}
	}

	Optional<RectF> Node::getChildrenContentRect() const
	{
		if (m_children.empty())
		{
			return none;
		}

		bool exists = false;
		double left = std::numeric_limits<double>::infinity();
		double top = std::numeric_limits<double>::infinity();
		double right = -std::numeric_limits<double>::infinity();
		double bottom = -std::numeric_limits<double>::infinity();
		for (const auto& child : m_children)
		{
			if (!child->isParentLayoutAffected())
			{
				continue;
			}
			exists = true;

			const RectF& childRect = child->layoutAppliedRect();
			left = Min(left, childRect.x);
			top = Min(top, childRect.y);
			right = Max(right, childRect.x + childRect.w);
			bottom = Max(bottom, childRect.y + childRect.h);
		}

		if (!exists)
		{
			return none;
		}

		return RectF{ left, top, right - left, bottom - top };
	}

	Optional<RectF> Node::getChildrenContentRectWithPadding() const
	{
		const auto contentRectOpt = getChildrenContentRect();
		if (!contentRectOpt)
		{
			return none;
		}
		const RectF& contentRect = *contentRectOpt;
		const LRTB& padding = layoutPadding();
		return RectF{ contentRect.x - padding.left, contentRect.y - padding.top, contentRect.w + padding.left + padding.right, contentRect.h + padding.top + padding.bottom };
	}

	std::shared_ptr<Node> Node::hoveredNodeInChildren()
	{
		// interactableはチェック不要(無効時も裏側をクリック不可にするためにホバー扱いにする必要があるため)
		if (!m_activeSelf)
		{
			return nullptr;
		}
		const bool mouseOver = m_effectedRect.mouseOver();
		if (m_clippingEnabled && !mouseOver)
		{
			return nullptr;
		}
		for (auto it = m_children.rbegin(); it != m_children.rend(); ++it)
		{
			if (const auto hoveredNode = (*it)->hoveredNodeInChildren())
			{
				return hoveredNode;
			}
		}
		if (m_isHitTarget && mouseOver)
		{
			return shared_from_this();
		}
		return nullptr;
	}

	std::shared_ptr<Node> Node::findContainedScrollableNode()
	{
		if (horizontalScrollable() || verticalScrollable())
		{
			return shared_from_this();
		}
		if (const auto parent = m_parent.lock())
		{
			return parent->findContainedScrollableNode();
		}
		return nullptr;
	}

	void Node::update(CanvasUpdateContext* pContext, const std::shared_ptr<Node>& hoveredNode, double deltaTime, const Mat3x2& parentEffectMat, const Vec2& parentEffectScale, InteractableYN parentInteractable, InteractState parentInteractState, InteractState parentInteractStateRight)
	{
		const auto thisNode = shared_from_this();

		m_currentInteractState = updateForCurrentInteractState(hoveredNode, parentInteractable);
		m_currentInteractStateRight = updateForCurrentInteractStateRight(hoveredNode, parentInteractable);
		if (!m_isHitTarget)
		{
			// Hovered対象でない場合は親のinteractStateを引き継ぐ
			m_currentInteractState = ApplyOtherInteractState(m_currentInteractState, parentInteractState);
			m_currentInteractStateRight = ApplyOtherInteractState(m_currentInteractStateRight, parentInteractStateRight);
		}
		for (const auto& component : m_components)
		{
			component->updateProperties(m_currentInteractState, m_selected, deltaTime);
		}
		if (!m_prevActiveInHierarchy.has_value() || m_activeInHierarchy.getBool() != m_prevActiveInHierarchy->getBool()) // YesNoにopetator==がないのでgetBool()を使っている
		{
			if (m_activeInHierarchy)
			{
				const auto guard = m_componentsIterGuard.scoped();
				for (const auto& component : m_components)
				{
					component->onActivated(pContext, thisNode);
				}
			}
			else
			{
				const auto guard = m_componentsIterGuard.scoped();
				for (const auto& component : m_components)
				{
					component->onDeactivated(pContext, thisNode);
				}
			}
		}
		if (m_activeInHierarchy)
		{
			const auto guard = m_componentsIterGuard.scoped();
			for (const auto& component : m_components)
			{
				component->update(pContext, thisNode);
			}
			m_transformEffect.update(m_currentInteractState, m_selected, deltaTime);
			refreshEffectedRect(parentEffectMat, parentEffectScale);
		}
		else
		{
			const auto guard = m_componentsIterGuard.scoped();
			for (const auto& component : m_components)
			{
				component->updateInactive(pContext, thisNode);
			}
		}
		{
			const auto guard = m_childrenIterGuard.scoped();
			const InteractableYN interactable{ m_interactable && parentInteractable };
			for (const auto& child : m_children)
			{
				child->update(pContext, hoveredNode, deltaTime, m_transformEffect.effectMat(parentEffectMat, m_layoutAppliedRect), m_transformEffect.scale().value() * parentEffectScale, interactable, m_currentInteractState, m_currentInteractStateRight);
			}
		}
		m_prevActiveInHierarchy = m_activeInHierarchy;
	}

	void Node::refreshEffectedRect(const Mat3x2& parentEffectMat, const Vec2& parentEffectScale)
	{
		const Mat3x2 effectMat = m_transformEffect.effectMat(parentEffectMat, m_layoutAppliedRect);
		const Vec2 posLeftTop = effectMat.transformPoint(m_layoutAppliedRect.pos);
		const Vec2 posRightBottom = effectMat.transformPoint(m_layoutAppliedRect.br());
		m_effectedRect = RectF{ posLeftTop, posRightBottom - posLeftTop };
		m_effectScale = parentEffectScale * m_transformEffect.scale().value();
		for (const auto& child : m_children)
		{
			child->refreshEffectedRect(effectMat, m_effectScale);
		}
	}

	void Node::scroll(const Vec2& offsetDelta, RefreshesLayoutYN refreshesLayout)
	{
		bool scrolledH = false;
		if (horizontalScrollable() && offsetDelta.x != 0.0)
		{
			m_scrollOffset.x += offsetDelta.x;
			scrolledH = true;
		}
		bool scrolledV = false;
		if (verticalScrollable() && offsetDelta.y != 0.0)
		{
			m_scrollOffset.y += offsetDelta.y;
			scrolledV = true;
		}
		if (scrolledH || scrolledV)
		{
			clampScrollOffset();
			if (refreshesLayout)
			{
				refreshContainedCanvasLayout();
			}
		}

		// 一定時間スクロールバーを表示
		// (レイアウト更新に時間がかかる場合を考慮して最後に実行)
		if (scrolledH)
		{
			m_scrollBarTimerH.restart();
		}
		if (scrolledV)
		{
			m_scrollBarTimerV.restart();
		}
	}

	void Node::draw() const
	{
		if (!m_activeSelf || !m_activeInHierarchy)
		{
			return;
		}

		// クリッピング有効の場合はクリッピング範囲を設定
		Optional<ScopedRenderStates2D> renderStates;
		if (m_clippingEnabled)
		{
			Graphics2D::SetScissorRect(m_effectedRect.asRect());
			RasterizerState rs = RasterizerState::Default2D;
			rs.scissorEnable = true;
			renderStates.emplace(rs);
		}

		{
			const auto guard = m_componentsIterGuard.scoped();
			for (const auto& component : m_components)
			{
				component->draw(*this);
			}
		}
		{
			const auto guard = m_childrenIterGuard.scoped();
			for (const auto& child : m_children)
			{
				child->draw();
			}
		}

		// スクロールバー描画
		const bool needHorizontalScrollBar = (horizontalScrollable() && m_scrollBarTimerH.isRunning());
		const bool needVerticalScrollBar = (verticalScrollable() && m_scrollBarTimerV.isRunning());
		if (needHorizontalScrollBar || needVerticalScrollBar)
		{
			if (const Optional<RectF> contentRectOpt = getChildrenContentRectWithPadding())
			{
				const RectF& contentRectLocal = *contentRectOpt;
				const Vec2 scale = m_effectScale;

				// 横スクロールバー
				if (needHorizontalScrollBar)
				{
					const double viewWidth = m_layoutAppliedRect.w * scale.x;
					const double contentWidth = contentRectLocal.w * scale.x;
					const double maxScrollX = (contentWidth > viewWidth) ? (contentWidth - viewWidth) : 0.0;
					if (maxScrollX > 0.0)
					{
						const double w = (viewWidth * viewWidth) / contentWidth;
						const double x = ((m_scrollOffset.x * scale.x) / maxScrollX) * (viewWidth - w);
						const double thickness = 4.0 * scale.x;
						const RectF rect
						{
							m_effectedRect.x + x,
							m_effectedRect.y + m_effectedRect.h - thickness,
							w,
							thickness
						};
						const double lerpRate = Max(m_scrollBarTimerH.progress0_1() - 0.75, 0.0) / 0.25;
						const double roundRadius = 2.0 * (scale.x + scale.y) / 2;
						rect.rounded(roundRadius).draw(ColorF(0.7, Math::Lerp(0.5, 0.0, lerpRate)));
					}
				}

				// 縦スクロールバー
				if (needVerticalScrollBar)
				{
					const double viewHeight = m_layoutAppliedRect.h * scale.y;
					const double contentHeight = contentRectLocal.h * scale.y;
					const double maxScrollY = (contentHeight > viewHeight) ? (contentHeight - viewHeight) : 0.0;
					if (maxScrollY > 0.0)
					{
						const double h = (viewHeight * viewHeight) / contentHeight;
						const double y = ((m_scrollOffset.y * scale.y) / maxScrollY) * (viewHeight - h);
						const double thickness = 4.0 * scale.x;
						const RectF rect
						{
							m_effectedRect.x + m_effectedRect.w - thickness,
							m_effectedRect.y + y,
							thickness,
							h
						};
						const double lerpRate = Max(m_scrollBarTimerV.progress0_1() - 0.75, 0.0) / 0.25;
						const double roundRadius = 2.0 * (scale.x + scale.y) / 2;
						rect.rounded(roundRadius).draw(ColorF(0.7, Math::Lerp(0.5, 0.0, lerpRate)));
					}
				}
			}
		}
	}

	const String& Node::name() const
	{
		return m_name;
	}

	void Node::setName(StringView name)
	{
		m_name = name;
	}

	const RectF& Node::rect() const
	{
		return m_effectedRect;
	}

	const Vec2& Node::effectScale() const
	{
		return m_effectScale;
	}

	const RectF& Node::layoutAppliedRect() const
	{
		return m_layoutAppliedRect;
	}

	const Array<std::shared_ptr<Node>>& Node::children() const
	{
		return m_children;
	}

	bool Node::hasChildren() const
	{
		return !m_children.isEmpty();
	}

	const Array<std::shared_ptr<ComponentBase>>& Node::components() const
	{
		return m_components;
	}

	InteractableYN Node::interactable() const
	{
		return m_interactable;
	}

	void Node::setInteractable(InteractableYN interactable)
	{
		m_interactable = interactable;
		m_mouseLTracker.setInteractable(interactable);
		m_mouseRTracker.setInteractable(interactable);
	}

	void Node::setInteractable(bool interactable)
	{
		setInteractable(InteractableYN{ interactable });
	}

	ActiveYN Node::activeSelf() const
	{
		return m_activeSelf;
	}

	void Node::setActive(ActiveYN activeSelf, RefreshesLayoutYN refreshesLayout)
	{
		m_activeSelf = activeSelf;
		refreshActiveInHierarchy();
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
	}

	void Node::setActive(bool activeSelf, RefreshesLayoutYN refreshesLayout)
	{
		setActive(ActiveYN{ activeSelf }, refreshesLayout);
	}

	ActiveYN Node::activeInHierarchy() const
	{
		return m_activeInHierarchy;
	}

	IsHitTargetYN Node::isHitTarget() const
	{
		return m_isHitTarget;
	}

	void Node::setIsHitTarget(IsHitTargetYN isHitTarget)
	{
		m_isHitTarget = isHitTarget;
	}

	void Node::setIsHitTarget(bool isHitTarget)
	{
		setIsHitTarget(IsHitTargetYN{ isHitTarget });
	}

	InheritChildrenStateFlags Node::inheritChildrenStateFlags() const
	{
		return m_inheritChildrenStateFlags;
	}

	void Node::setInheritChildrenStateFlags(InheritChildrenStateFlags flags)
	{
		m_inheritChildrenStateFlags = flags;
	}

	bool Node::inheritsChildrenHoveredState() const
	{
		return HasFlag(m_inheritChildrenStateFlags, InheritChildrenStateFlags::Hovered);
	}

	void Node::setInheritsChildrenHoveredState(bool value)
	{
		if (value)
		{
			m_inheritChildrenStateFlags |= InheritChildrenStateFlags::Hovered;
		}
		else
		{
			m_inheritChildrenStateFlags &= ~InheritChildrenStateFlags::Hovered;
		}
	}

	bool Node::inheritsChildrenPressedState() const
	{
		return HasFlag(m_inheritChildrenStateFlags, InheritChildrenStateFlags::Pressed);
	}

	void Node::setInheritsChildrenPressedState(bool value)
	{
		if (value)
		{
			m_inheritChildrenStateFlags |= InheritChildrenStateFlags::Pressed;
		}
		else
		{
			m_inheritChildrenStateFlags &= ~InheritChildrenStateFlags::Pressed;
		}
	}

	ScrollableAxisFlags Node::scrollableAxisFlags() const
	{
		return m_scrollableAxisFlags;
	}

	void Node::setScrollableAxisFlags(ScrollableAxisFlags flags, RefreshesLayoutYN refreshesLayout)
	{
		m_scrollableAxisFlags = flags;
		if (!horizontalScrollable())
		{
			m_scrollOffset.x = 0.0;
		}
		if (!verticalScrollable())
		{
			m_scrollOffset.y = 0.0;
		}
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
	}

	bool Node::horizontalScrollable() const
	{
		return HasFlag(m_scrollableAxisFlags, ScrollableAxisFlags::Horizontal);
	}

	void Node::setHorizontalScrollable(bool scrollable, RefreshesLayoutYN refreshesLayout)
	{
		if (scrollable)
		{
			m_scrollableAxisFlags |= ScrollableAxisFlags::Horizontal;
		}
		else
		{
			m_scrollableAxisFlags &= ~ScrollableAxisFlags::Horizontal;
			m_scrollOffset.x = 0.0;
		}
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
	}

	bool Node::verticalScrollable() const
	{
		return HasFlag(m_scrollableAxisFlags, ScrollableAxisFlags::Vertical);
	}

	void Node::setVerticalScrollable(bool scrollable, RefreshesLayoutYN refreshesLayout)
	{
		if (scrollable)
		{
			m_scrollableAxisFlags |= ScrollableAxisFlags::Vertical;
		}
		else
		{
			m_scrollableAxisFlags &= ~ScrollableAxisFlags::Vertical;
			m_scrollOffset.y = 0.0;
		}
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
	}

	ClippingEnabledYN Node::clippingEnabled() const
	{
		return m_clippingEnabled;
	}

	void Node::setClippingEnabled(ClippingEnabledYN clippingEnabled)
	{
		m_clippingEnabled = clippingEnabled;
	}

	void Node::setClippingEnabled(bool clippingEnabled)
	{
		setClippingEnabled(ClippingEnabledYN{ clippingEnabled });
	}

	InteractState Node::interactStateSelf() const
	{
		return m_mouseLTracker.interactStateSelf();
	}

	InteractState Node::currentInteractState() const
	{
		return m_currentInteractState;
	}

	SelectedYN Node::selected() const
	{
		return m_selected;
	}

	void Node::setSelected(SelectedYN selected)
	{
		m_selected = selected;
	}

	void Node::setSelected(bool selected)
	{
		setSelected(SelectedYN{ selected });
	}

	bool Node::isHovered() const
	{
		return m_mouseLTracker.isHovered();
	}

	bool Node::isHoveredRecursive() const
	{
		return isHovered() || m_children.any([](const auto& child) { return child->isHoveredRecursive(); });
	}

	bool Node::isPressed() const
	{
		return m_mouseLTracker.isPressed();
	}

	bool Node::isPressedRecursive() const
	{
		return isPressed() || m_children.any([](const auto& child) { return child->isPressedRecursive(); });
	}

	bool Node::isPressedHover() const
	{
		return m_mouseLTracker.isPressedHover();
	}

	bool Node::isPressedHoverRecursive() const
	{
		return isPressedHover() || m_children.any([](const auto& child) { return child->isPressedHoverRecursive(); });
	}

	bool Node::isMouseDown() const
	{
		return m_mouseLTracker.isHovered() && MouseL.down();
	}

	bool Node::isMouseDownRecursive() const
	{
		return isMouseDown() || m_children.any([](const auto& child) { return child->isMouseDownRecursive(); });
	}

	bool Node::isClicked() const
	{
		return m_mouseLTracker.isClicked();
	}

	bool Node::isClickedRecursive() const
	{
		return isClicked() || m_children.any([](const auto& child) { return child->isClickedRecursive(); });
	}

	bool Node::isRightPressed() const
	{
		return m_mouseRTracker.isPressed();
	}

	bool Node::isRightPressedRecursive() const
	{
		return isRightPressed() || m_children.any([](const auto& child) { return child->isRightPressedRecursive(); });
	}

	bool Node::isRightPressedHover() const
	{
		return m_mouseRTracker.isPressedHover();
	}

	bool Node::isRightPressedHoverRecursive() const
	{
		return isRightPressedHover() || m_children.any([](const auto& child) { return child->isRightPressedHoverRecursive(); });
	}

	bool Node::isRightMouseDown() const
	{
		return m_mouseRTracker.isHovered() && MouseR.down();
	}

	bool Node::isRightMouseDownRecursive() const
	{
		return isRightMouseDown() || m_children.any([](const auto& child) { return child->isRightMouseDownRecursive(); });
	}

	bool Node::isRightClicked() const
	{
		return m_mouseRTracker.isClicked();
	}

	bool Node::isRightClickedRecursive() const
	{
		return isRightClicked() || m_children.any([](const auto& child) { return child->isRightClickedRecursive(); });
	}

	void Node::removeChildrenAll(RefreshesLayoutYN refreshesLayout)
	{
		if (m_childrenIterGuard.isIterating())
		{
			throw Error{ U"removeChildrenAll: Cannot remove children while iterating" };
		}
		for (const auto& child : m_children)
		{
			child->setCanvasRecursive(std::weak_ptr<Canvas>{});
			child->m_parent.reset();
			child->refreshActiveInHierarchy();
		}
		m_children.clear();
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
	}

	void Node::swapChildren(const std::shared_ptr<Node>& child1, const std::shared_ptr<Node>& child2, RefreshesLayoutYN refreshesLayout)
	{
		if (m_childrenIterGuard.isIterating())
		{
			throw Error{ U"swapChildren: Cannot swap children while iterating" };
		}
		const auto it1 = std::find(m_children.begin(), m_children.end(), child1);
		const auto it2 = std::find(m_children.begin(), m_children.end(), child2);
		if (it1 == m_children.end() || it2 == m_children.end())
		{
			throw Error{ U"swapChildren: Child node not found in node '{}'"_fmt(m_name) };
		}
		std::iter_swap(it1, it2);
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
	}

	void Node::swapChildren(size_t index1, size_t index2, RefreshesLayoutYN refreshesLayout)
	{
		if (m_childrenIterGuard.isIterating())
		{
			throw Error{ U"swapChildren: Cannot swap children while iterating" };
		}
		if (index1 >= m_children.size() || index2 >= m_children.size())
		{
			throw Error{ U"swapChildren: Index out of range" };
		}
		std::iter_swap(m_children.begin() + index1, m_children.begin() + index2);
		if (refreshesLayout)
		{
			refreshContainedCanvasLayout();
		}
	}

	std::shared_ptr<Node> Node::clone() const
	{
		return FromJSON(toJSON());
	}

	void Node::addUpdater(std::function<void(const std::shared_ptr<Node>&)> updater)
	{
		emplaceComponent<UpdaterComponent>(std::move(updater));
	}

	void Node::addDrawer(std::function<void(const Node&)> drawer)
	{
		emplaceComponent<DrawerComponent>(std::move(drawer));
	}

	void Node::addOnClick(std::function<void(const std::shared_ptr<Node>&)> onClick)
	{
		emplaceComponent<UpdaterComponent>([onClick = std::move(onClick)](const std::shared_ptr<Node>& node)
			{
				if (node->isClicked())
				{
					onClick(node);
				}
			});
	}

	void Node::addOnRightClick(std::function<void(const std::shared_ptr<Node>&)> onRightClick)
	{
		emplaceComponent<UpdaterComponent>([onRightClick = std::move(onRightClick)](const std::shared_ptr<Node>& node)
			{
				if (node->isRightClicked())
				{
					onRightClick(node);
				}
			});
	}

	void Node::refreshContainedCanvasLayout()
	{
		if (const auto canvas = m_canvas.lock())
		{
			canvas->refreshLayout();
		}
	}
}
