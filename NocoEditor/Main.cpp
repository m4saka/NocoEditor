﻿#include <Siv3D.hpp>
#include "NocoUI.hpp"

using namespace noco;

using CheckedYN = YesNo<struct CheckedYN_tag>;

constexpr int32 MenuBarHeight = 26;

struct MenuItem
{
	String text;
	String hotKeyText;
	std::function<void()> onClick = nullptr;
	std::function<bool()> fnIsEnabled = [] { return true; };
};

struct CheckableMenuItem
{
	String text;
	String hotKeyText;
	std::function<void(CheckedYN)> onClick = nullptr;
	CheckedYN checked = CheckedYN::No;
	std::function<bool()> fnIsEnabled = [] { return true; };
};

struct MenuSeparator
{
};

using MenuElement = std::variant<MenuItem, CheckableMenuItem, MenuSeparator>;

[[nodiscard]]
static PropertyValue<ColorF> MenuItemRectFillColor()
{
	return PropertyValue<ColorF>{ ColorF{ 0.8, 0.0 }, ColorF{ 0.8 }, ColorF{ 0.8 }, ColorF{ 0.8, 0.0 }, 0.05 };
}

class ContextMenu
{
public:
	static constexpr int32 MenuItemWidth = 300;
	static constexpr int32 MenuItemHeight = 30;

private:
	std::shared_ptr<Canvas> m_editorOverlayCanvas;
	std::shared_ptr<Node> m_rootNode;
	std::shared_ptr<Node> m_menuNode;

	Array<MenuElement> m_elements;
	Array<std::shared_ptr<Node>> m_elementNodes;

	std::function<void()> m_fnOnHide = nullptr;

	bool m_isFirstUpdateSinceShown = false;

	void clearItems()
	{
		m_elements.clear();
		m_elementNodes.clear();
		m_menuNode->setActive(ActiveYN::No);
		m_menuNode->removeChildrenAll();
		m_fnOnHide = nullptr;
	}

public:
	explicit ContextMenu(const std::shared_ptr<Canvas>& editorOverlayCanvas, StringView name = U"ContextMenu")
		: m_editorOverlayCanvas(editorOverlayCanvas)
		, m_rootNode(editorOverlayCanvas->rootNode()->emplaceChild(
			U"{}_Root"_fmt(name),
			AnchorConstraint
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::BottomRight,
				.posDelta = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ 0, 0 },
				.pivot = Anchor::TopLeft,
			}))
			, m_menuNode(m_rootNode->emplaceChild(
				name,
				BoxConstraint
				{
					.sizeRatio = Vec2{ 0, 0 },
					.sizeDelta = Vec2{ MenuItemWidth, 0 },
				}))
	{
		m_rootNode->setActive(ActiveYN::No, RefreshesLayoutYN::No);

		m_menuNode->setLayout(VerticalLayout{}, RefreshesLayoutYN::No);
		m_menuNode->setVerticalScrollable(true, RefreshesLayoutYN::No);
		m_menuNode->emplaceComponent<RectRenderer>(ColorF{ 0.95 }, Palette::Black, 0.0, 0.0, ColorF{ 0.0, 0.4 }, Vec2{ 2, 2 }, 5);
		m_menuNode->setActive(ActiveYN::No, RefreshesLayoutYN::No);

		editorOverlayCanvas->refreshLayout();
	}

	void show(const Vec2& pos, const Array<MenuElement>& elements, std::function<void()> fnOnHide = nullptr)
	{
		// 前回開いていたメニューを閉じてから表示
		hide(RefreshesLayoutYN::No);
		m_elements = elements;
		m_elementNodes.reserve(m_elements.size());
		m_fnOnHide = std::move(fnOnHide);

		for (size_t i = 0; i < m_elements.size(); ++i)
		{
			if (const auto pItem = std::get_if<MenuItem>(&m_elements[i]))
			{
				// 項目
				const auto itemNode = m_menuNode->emplaceChild(
					U"MenuItem_{}"_fmt(i),
					BoxConstraint
					{
						.sizeRatio = Vec2{ 1, 0 },
						.sizeDelta = Vec2{ 0, MenuItemHeight },
					},
					IsHitTargetYN::Yes,
					InheritChildrenStateFlags::None,
					RefreshesLayoutYN::No);
				itemNode->emplaceComponent<RectRenderer>(MenuItemRectFillColor());
				itemNode->emplaceComponent<Label>(pItem->text, U"Font14", 14, PropertyValue<ColorF>{ ColorF{ 0.0 } }.withDisabled(ColorF{ 0.5 }), HorizontalAlign::Left, VerticalAlign::Middle, LRTB{ 30, 10, 0, 0 });
				if (!pItem->hotKeyText.empty())
				{
					itemNode->emplaceComponent<Label>(pItem->hotKeyText, U"Font14", 14, PropertyValue<ColorF>{ ColorF{ 0.0 } }.withDisabled(ColorF{ 0.5 }), HorizontalAlign::Right, VerticalAlign::Middle, LRTB{ 0, 10, 0, 0 });
				}
				itemNode->setInteractable(pItem->fnIsEnabled());
				m_elementNodes.push_back(itemNode);
			}
			else if (const auto pCheckableItem = std::get_if<CheckableMenuItem>(&m_elements[i]))
			{
				// チェック可能な項目
				const auto itemNode = m_menuNode->emplaceChild(
					U"CheckableMenuItem_{}"_fmt(i),
					BoxConstraint
					{
						.sizeRatio = Vec2{ 1, 0 },
						.sizeDelta = Vec2{ 0, MenuItemHeight },
					},
					IsHitTargetYN::Yes,
					InheritChildrenStateFlags::None,
					RefreshesLayoutYN::No);
				itemNode->emplaceComponent<RectRenderer>(MenuItemRectFillColor());
				itemNode->emplaceComponent<Label>(pCheckableItem->text, U"Font14", 14, PropertyValue<ColorF>{ ColorF{ 0.0 } }.withDisabled(ColorF{ 0.5 }), HorizontalAlign::Left, VerticalAlign::Middle, LRTB{ 30, 10, 0, 0 });
				if (!pCheckableItem->hotKeyText.empty())
				{
					itemNode->emplaceComponent<Label>(pCheckableItem->hotKeyText, U"Font14", 14, PropertyValue<ColorF>{ ColorF{ 0.0 } }.withDisabled(ColorF{ 0.5 }), HorizontalAlign::Right, VerticalAlign::Middle, LRTB{ 0, 10, 0, 0 });
				}
				itemNode->emplaceComponent<Label>(
					pCheckableItem->checked ? U"✔" : U"",
					U"Font14",
					14,
					PropertyValue<ColorF>{ ColorF{ 0.0 } }.withDisabled(ColorF{ 0.5 }),
					HorizontalAlign::Left,
					VerticalAlign::Middle,
					LRTB{ 10, 10, 0, 0 });
				itemNode->setInteractable(pCheckableItem->fnIsEnabled());
				m_elementNodes.push_back(itemNode);
			}
			else if (std::holds_alternative<MenuSeparator>(m_elements[i]))
			{
				// セパレータ
				const auto separatorNode = m_menuNode->emplaceChild(
					U"Separator",
					BoxConstraint
					{
						.sizeRatio = Vec2{ 1, 0 },
						.sizeDelta = Vec2{ 0, 8 },
					},
					IsHitTargetYN::No,
					InheritChildrenStateFlags::None,
					RefreshesLayoutYN::No);
				separatorNode->emplaceChild(
					U"SeparatorLine",
					AnchorConstraint
					{
						.anchorMin = Anchor::MiddleLeft,
						.anchorMax = Anchor::MiddleRight,
						.sizeDelta = Vec2{ -10, 1 },
						.pivot = Anchor::MiddleCenter,
					},
					IsHitTargetYN::No)
					->emplaceComponent<RectRenderer>(ColorF{ 0.7 });

				m_elementNodes.push_back(separatorNode);
			}
		}

		double x = pos.x;
		double y = pos.y;

		// 右端にはみ出す場合は左に寄せる
		if (x + MenuItemWidth > Scene::Width())
		{
			x = Scene::Width() - MenuItemWidth;
		}

		m_menuNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly, RefreshesLayoutYN::No);
		m_menuNode->setActive(ActiveYN::Yes, RefreshesLayoutYN::No);

		// 下端にはみ出す場合は上に寄せる
		const double menuHeight = m_menuNode->boxConstraint()->sizeDelta.y;
		if (y + menuHeight > Scene::Height())
		{
			y = Scene::Height() - menuHeight;
		}

		if (const auto pAnchorConstraint = m_rootNode->anchorConstraint())
		{
			auto newConstraint = *pAnchorConstraint;
			newConstraint.posDelta = Vec2{ x, y };
			m_rootNode->setConstraint(newConstraint, RefreshesLayoutYN::No);
		}

		m_rootNode->setActive(ActiveYN::Yes, RefreshesLayoutYN::Yes);

		m_isFirstUpdateSinceShown = true;
	}

	void hide(RefreshesLayoutYN refreshesLayout = RefreshesLayoutYN::Yes)
	{
		if (m_fnOnHide)
		{
			m_fnOnHide();
			m_fnOnHide = nullptr;
		}
		clearItems();
		m_rootNode->setActive(ActiveYN::No, refreshesLayout);
		m_isFirstUpdateSinceShown = false;
	}

	void update()
	{
		// 初回フレームは閉じる判定しない
		if (!m_menuNode->activeSelf() || m_isFirstUpdateSinceShown)
		{
			m_isFirstUpdateSinceShown = false;
			return;
		}

		// メニュー項目クリック判定
		for (size_t i = 0; i < m_elements.size(); ++i)
		{
			if (const auto pItem = std::get_if<MenuItem>(&m_elements[i]))
			{
				if (m_elementNodes[i]->isClicked())
				{
					if (pItem->onClick)
					{
						pItem->onClick();
						hide();
						break;
					}
				}
			}
			else if (const auto pCheckableItem = std::get_if<CheckableMenuItem>(&m_elements[i]))
			{
				if (m_elementNodes[i]->isClicked())
				{
					if (pCheckableItem->onClick)
					{
						pCheckableItem->onClick(CheckedYN{ !pCheckableItem->checked });
						// pCheckableItemは下記のhideで即座に解放されるため、ここでのpCheckableItemのチェック状態の書き換えは不要
						hide();
						break;
					}
				}
			}
		}

		// メニュー外クリックで閉じる
		// (無効項目のクリックで消えないようにするため、rectがmouseOverしていないこともチェック)
		if (!m_menuNode->isHoveredRecursive() && !m_menuNode->rect().mouseOver() && (MouseL.down() || MouseM.down() || MouseR.down()))
		{
			hide();
		}
	}

	[[nodiscard]]
	bool isHoveredRecursive() const
	{
		return m_rootNode->isHoveredRecursive();
	}
};

class ContextMenuOpener : public ComponentBase
{
private:
	std::shared_ptr<ContextMenu> m_contextMenu;
	Array<MenuElement> m_menuElements;
	std::function<void()> m_fnBeforeOpen;

public:
	explicit ContextMenuOpener(const std::shared_ptr<ContextMenu>& contextMenu, Array<MenuElement> menuElements, std::function<void()> fnBeforeOpen = nullptr)
		: ComponentBase{ U"ContextMenuOpener", {} }
		, m_contextMenu{ contextMenu }
		, m_menuElements{ std::move(menuElements) }
		, m_fnBeforeOpen{ std::move(fnBeforeOpen) }
	{
	}

	void update(CanvasUpdateContext*, const std::shared_ptr<Node>& node) override
	{
		if (node->isRightClicked())
		{
			if (m_fnBeforeOpen)
			{
				m_fnBeforeOpen();
			}
			m_contextMenu->show(Cursor::PosF(), m_menuElements);
		}
	}

	void draw(const Node&) const override
	{
	}
};

struct MenuCategory
{
	String text;
	Array<MenuElement> elements;
	std::shared_ptr<Node> node;
};

class MenuBar
{
private:
	std::shared_ptr<Canvas> m_editorCanvas;
	std::shared_ptr<Node> m_menuBarRootNode;
	Array<MenuCategory> m_menuCategories;
	std::shared_ptr<ContextMenu> m_contextMenu;
	std::shared_ptr<Node> m_activeMenuCategoryNode;
	bool m_hasMenuClosed = false;

public:
	explicit MenuBar(const std::shared_ptr<Canvas>& editorCanvas, const std::shared_ptr<ContextMenu>& contextMenu)
		: m_editorCanvas(editorCanvas)
		, m_menuBarRootNode(editorCanvas->rootNode()->emplaceChild(
			U"MenuBar",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::TopRight,
				.posDelta = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ 0, MenuBarHeight },
				.pivot = Anchor::TopLeft,
			}))
		, m_contextMenu(contextMenu)
	{
		m_menuBarRootNode->setLayout(HorizontalLayout{});
		m_menuBarRootNode->emplaceComponent<RectRenderer>(ColorF{ 0.95 });
	}

	void addMenuCategory(StringView name, StringView text, const Array<MenuElement>& elements)
	{
		const auto node = m_menuBarRootNode->emplaceChild(
			name,
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.sizeDelta = Vec2{ 80, 0 },
			});
		node->emplaceComponent<RectRenderer>(MenuItemRectFillColor());
		node->emplaceComponent<Label>(text, U"Font14", 14, PropertyValue<ColorF>{ ColorF{ 0.0 } }.withDisabled(ColorF{ 0.0, 0.5 }), HorizontalAlign::Center, VerticalAlign::Middle);

		m_menuCategories.push_back(MenuCategory
		{
			.text = String{ text },
			.elements = elements,
			.node = node,
		});
	}

	void update()
	{
		bool hasMenuOpened = false;
		for (const auto& menuCategory : m_menuCategories)
		{
			if (menuCategory.node->isMouseDown())
			{
				if (m_activeMenuCategoryNode == menuCategory.node)
				{
					// 同じメニューが再度クリックされた場合は非表示
					m_contextMenu->hide();
				}
				else
				{
					// メニューがクリックされた場合は表示を切り替え
					m_contextMenu->show(menuCategory.node->rect().bl(), menuCategory.elements, [this] { m_hasMenuClosed = true; });
					m_activeMenuCategoryNode = menuCategory.node;
					hasMenuOpened = true;
				}
			}
			else if (menuCategory.node->isHoveredRecursive() && m_activeMenuCategoryNode && m_activeMenuCategoryNode != menuCategory.node)
			{
				// カーソルが他のメニューに移動した場合はサブメニューを切り替える
				m_contextMenu->show(menuCategory.node->rect().bl(), menuCategory.elements, [this] { m_hasMenuClosed = true; });
				m_activeMenuCategoryNode = menuCategory.node;
				hasMenuOpened = true;
			}
		}

		if (m_hasMenuClosed && !hasMenuOpened)
		{
			m_activeMenuCategoryNode = nullptr;
		}
		m_hasMenuClosed = false;
	}
};

class Hierarchy
{
private:
	std::shared_ptr<Canvas> m_canvas;
	std::shared_ptr<Node> m_hierarchyFrameNode;
	std::shared_ptr<Node> m_hierarchyInnerFrameNode;
	std::shared_ptr<Node> m_hierarchyRootNode;
	std::weak_ptr<Canvas> m_editorCanvas;
	std::weak_ptr<Node> m_editorHoveredNode;
	std::weak_ptr<Node> m_shiftSelectOriginNode;
	std::weak_ptr<Node> m_lastEditorSelectedNode;
	std::shared_ptr<ContextMenu> m_contextMenu;
	Array<JSON> m_copiedNodeJSONs;

	struct ElementDetail
	{
		size_t nestLevel = 0;
		std::shared_ptr<Node> node;
		std::shared_ptr<Node> hierarchyNode;
		std::shared_ptr<RectRenderer> hierarchyRectRenderer;
		std::shared_ptr<Label> hierarchyStateLabel;
		std::shared_ptr<Node> hierarchyToggleFoldedNode;
		std::shared_ptr<Label> hierarchyToggleFoldedLabel;
	};

	class Element
	{
	private:
		Hierarchy* m_pHierarchy = nullptr;
		ElementDetail m_elementDetail;
		EditorSelectedYN m_editorSelected = EditorSelectedYN::No;
		FoldedYN m_folded = FoldedYN::No;

	public:
		Element(Hierarchy* pHierarchy, const ElementDetail& elementDetail)
			: m_pHierarchy{ pHierarchy }
			, m_elementDetail{ elementDetail }
		{
			if (m_pHierarchy == nullptr)
			{
				throw Error{ U"Hierarchy is nullptr" };
			}
		}

		[[nodiscard]]
		EditorSelectedYN editorSelected() const
		{
			return m_editorSelected;
		}

		void setEditorSelected(EditorSelectedYN editorSelected)
		{
			m_editorSelected = editorSelected;
			m_elementDetail.hierarchyRectRenderer->setFillColor(HierarchyRectFillColor(m_editorSelected));
			m_elementDetail.hierarchyRectRenderer->setOutlineColor(HierarchyRectOutlineColor(m_editorSelected));
		}

		[[nodiscard]]
		const ElementDetail& elementDetail() const
		{
			return m_elementDetail;
		}

		[[nodiscard]]
		const std::shared_ptr<Node>& node() const
		{
			return m_elementDetail.node;
		}

		[[nodiscard]]
		const std::shared_ptr<Node>& hierarchyNode() const
		{
			return m_elementDetail.hierarchyNode;
		}

		void toggleFolded()
		{
			setFolded(m_folded ? FoldedYN::No : FoldedYN::Yes);
		}

		void setFolded(FoldedYN folded)
		{
			m_folded = folded;
			if (m_folded)
			{
				m_elementDetail.hierarchyToggleFoldedLabel->setText(U"▶");
			}
			else
			{
				m_elementDetail.hierarchyToggleFoldedLabel->setText(U"▼");
			}
			m_pHierarchy->applyFolding();
		}

		[[nodiscard]]
		FoldedYN folded() const
		{
			return m_folded;
		}

		[[nodiscard]]
		static PropertyValue<ColorF> HierarchyRectFillColor(EditorSelectedYN editorSelected)
		{
			if (editorSelected)
			{
				return ColorF{ Palette::Orange, 0.3 };
			}
			else
			{
				return PropertyValue<ColorF>{ ColorF{ 1.0, 0.0 } }.withHover(ColorF{ 1.0, 0.2 });
			}
		}

		[[nodiscard]]
		static PropertyValue<ColorF> HierarchyRectOutlineColor(EditorSelectedYN editorSelected)
		{
			if (editorSelected)
			{
				return ColorF{ Palette::Orange, 0.6 };
			}
			else
			{
				return PropertyValue<ColorF>{ ColorF{ 1.0, 0.0 } }.withHover(ColorF{ 1.0, 0.6 });
			}
		}
	};
	Array<Element> m_elements;

	void addElementRecursive(const std::shared_ptr<Node>& node, size_t nestLevel, RefreshesLayoutYN refreshesLayout)
	{
		if (node == nullptr)
		{
			throw Error{ U"Node is nullptr" };
		}

		m_elements.push_back(createElement(node, nestLevel));
		m_hierarchyRootNode->addChild(m_elements.back().elementDetail().hierarchyNode, refreshesLayout);

		for (const auto& child : node->children())
		{
			addElementRecursive(child, nestLevel + 1, refreshesLayout);
		}
	}

	Element createElement(const std::shared_ptr<Node>& node, size_t nestLevel)
	{
		const auto hierarchyNode = Node::Create(
			U"Element",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 24 },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered);
		hierarchyNode->emplaceComponent<ContextMenuOpener>(
			m_contextMenu,
			Array<MenuElement>
			{
				MenuItem{ U"新規ノード", U"", [this] { onClickNewNode(); } },
				MenuItem{ U"子として新規ノード", U"", [this, node] { onClickNewNode(node); } },
				MenuSeparator{},
				MenuItem{ U"切り取り", U"Ctrl+X", [this] { onClickCut(); } },
				MenuItem{ U"コピー", U"Ctrl+C", [this] { onClickCopy(); } },
				MenuItem{ U"貼り付け", U"Ctrl+V", [this] { onClickPaste(); }, [this] { return canPaste(); } },
				MenuItem{ U"子として貼り付け", U"", [this, node] { onClickPaste(node); }, [this] { return canPaste(); } },
				MenuItem{ U"複製を作成", U"Ctrl+D", [this] { onClickDuplicate(); } },
				MenuItem{ U"削除", U"Delete", [this] { onClickDelete(); } },
				MenuSeparator{},
				MenuItem{ U"上に移動", U"Alt+Up", [this] { onClickMoveUp(); } },
				MenuItem{ U"下に移動", U"Alt+Down", [this] { onClickMoveDown(); } },
				MenuSeparator{},
				MenuItem{ U"空の親ノードを作成", U"", [this] { onClickCreateEmptyParent(); } },
			},
			[this, node]
			{
				Element* const pElement = getElementByNode(node);
				if (pElement == nullptr)
				{
					throw Error{ U"Element not found" };
				}
				Element& element = *pElement;

				// 既に選択中の場合は何もしない
				if (element.editorSelected())
				{
					return;
				}
				clearSelection();
				element.setEditorSelected(EditorSelectedYN::Yes);
				m_lastEditorSelectedNode = node;
				m_shiftSelectOriginNode = node;
			});
		hierarchyNode->emplaceComponent<RectRenderer>(Element::HierarchyRectFillColor(EditorSelectedYN::No), Element::HierarchyRectOutlineColor(EditorSelectedYN::No), 1.0, 3.0);
		const auto nameLabel = hierarchyNode->emplaceComponent<Label>(
			node->name(),
			U"Font14Bold",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 20 + static_cast<double>(nestLevel) * 20, 5, 0, 0 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip);

		const auto stateLabel = hierarchyNode->emplaceComponent<Label>(
			U"",
			U"Font14",
			14,
			Palette::White,
			HorizontalAlign::Right,
			VerticalAlign::Middle,
			LRTB{ 0, 5, 0, 0 },
			HorizontalOverflow::Overflow,
			VerticalOverflow::Clip);

		const auto toggleFoldedNode = hierarchyNode->emplaceChild(
			U"ToggleFolded",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::BottomLeft,
				.posDelta = Vec2{ 10 + nestLevel * 20, 0 },
				.sizeDelta = Vec2{ 30, 0 },
				.pivot = Anchor::MiddleCenter,
			});
		toggleFoldedNode->setActive(node->hasChildren() ? ActiveYN::Yes : ActiveYN::No);
		toggleFoldedNode->addOnClick([this, node](const std::shared_ptr<Node>&)
			{
				if (!node->hasChildren())
				{
					// 子がないので折り畳み不可
					return;
				}

				if (const auto pElement = getElementByNode(node))
				{
					pElement->toggleFolded();
				}
			});
		const auto toggleFoldedLabel = toggleFoldedNode->emplaceComponent<Label>(
			U"▼",
			U"Font14",
			10,
			ColorF{ 1.0, 0.6 },
			HorizontalAlign::Center,
			VerticalAlign::Middle);

		return Element
		{
			this,
			ElementDetail
			{
				.nestLevel = nestLevel,
				.node = node,
				.hierarchyNode = hierarchyNode,
				.hierarchyRectRenderer = hierarchyNode->getComponent<RectRenderer>(),
				.hierarchyStateLabel = stateLabel,
				.hierarchyToggleFoldedNode = toggleFoldedNode,
				.hierarchyToggleFoldedLabel = toggleFoldedLabel,
			}
		};
	}

	Element* getElementByNode(const std::shared_ptr<Node>& node)
	{
		if (node == nullptr)
		{
			return nullptr;
		}
		const auto it = std::find_if(m_elements.begin(), m_elements.end(),
			[node](const auto& e) { return e.node() == node; });
		if (it == m_elements.end())
		{
			return nullptr;
		}
		return &(*it);
	}

	void applyFoldingRecursive(Element& element, FoldedYN parentFoldedInHierarchy)
	{
		// 親が折り畳まれている場合はHierarchy上で非表示にする
		element.hierarchyNode()->setActive(parentFoldedInHierarchy ? ActiveYN::No : ActiveYN::Yes);

		// 再帰的に適用
		for (auto& childNode : element.node()->children())
		{
			if (Element* childElement = getElementByNode(childNode))
			{
				applyFoldingRecursive(*childElement, FoldedYN{ parentFoldedInHierarchy || element.folded() });
			}
		}
	}

public:
	explicit Hierarchy(const std::shared_ptr<Canvas>& canvas, const std::shared_ptr<Canvas>& editorCanvas, const std::shared_ptr<ContextMenu>& contextMenu)
		: m_canvas(canvas)
		, m_hierarchyFrameNode(editorCanvas->rootNode()->emplaceChild(
			U"HierarchyFrame",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::BottomLeft,
				.posDelta = Vec2{ 0, MenuBarHeight },
				.sizeDelta = Vec2{ 300, -MenuBarHeight },
				.pivot = Anchor::TopLeft,
			}))
		, m_hierarchyInnerFrameNode(m_hierarchyFrameNode->emplaceChild(
			U"HierarchyInnerFrame",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::BottomRight,
				.posDelta = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ -2, -2 },
				.pivot = Anchor::MiddleCenter,
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered | InheritChildrenStateFlags::Pressed))
		, m_hierarchyRootNode(m_hierarchyInnerFrameNode->emplaceChild(
			U"Hierarchy",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::BottomRight,
				.posDelta = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ -10, -14 },
				.pivot = Anchor::MiddleCenter,
			}))
		, m_editorCanvas(editorCanvas)
		, m_contextMenu(contextMenu)
	{
		m_hierarchyFrameNode->emplaceComponent<RectRenderer>(ColorF{ 0.5, 0.4 }, Palette::Black, 0.0, 10.0);
		m_hierarchyInnerFrameNode->emplaceComponent<RectRenderer>(ColorF{ 0.1, 0.8 }, Palette::Black, 0.0, 10.0);
		m_hierarchyInnerFrameNode->emplaceComponent<ContextMenuOpener>(contextMenu,
			Array<MenuElement>
			{
				MenuItem{ U"新規ノード", U"", [this] { onClickNewNode(); } },
				MenuItem{ U"貼り付け", U"Ctrl+V", [this] { onClickPaste(); }, [this] { return canPaste(); } },
			});
		m_hierarchyRootNode->setLayout(VerticalLayout{ .padding = 2 });
		m_hierarchyRootNode->setVerticalScrollable(true);

		refreshNodeList();
	}

	void refreshNodeList()
	{
		Array<std::weak_ptr<Node>> foldedNodes;
		foldedNodes.reserve(m_elements.size());
		for (const auto& element : m_elements)
		{
			if (element.folded())
			{
				foldedNodes.push_back(element.node());
			}
		}

		clearSelection();
		m_elements.clear();
		m_hierarchyRootNode->removeChildrenAll();
		addElementRecursive(m_canvas->rootNode(), 0, RefreshesLayoutYN::Yes);

		for (const auto& node : foldedNodes)
		{
			if (auto pElement = getElementByNode(node.lock()))
			{
				pElement->setFolded(FoldedYN::Yes);
			}
		}
	}

	void refreshNodeNames()
	{
		for (const auto& element : m_elements)
		{
			element.hierarchyNode()->getComponent<Label>()->setText(element.node()->name());
		}
	}

	void selectNodes(const Array<std::shared_ptr<Node>>& nodes)
	{
		clearSelection();
		for (const auto& node : nodes)
		{
			if (auto pElement = getElementByNode(node))
			{
				pElement->setEditorSelected(EditorSelectedYN::Yes);
				unfoldForNode(node);
			}
		}
		if (nodes.size() == 1)
		{
			m_lastEditorSelectedNode = nodes.front();
			m_shiftSelectOriginNode = nodes.front();
		}
	}

	void selectAll()
	{
		if (m_elements.empty())
		{
			return;
		}

		for (auto& element : m_elements)
		{
			element.setEditorSelected(EditorSelectedYN::Yes);
		}

		m_lastEditorSelectedNode = m_elements.back().node();
		m_shiftSelectOriginNode = m_elements.front().node();
	}

	void selectSingleNode(const std::shared_ptr<Node>& node)
	{
		clearSelection();
		if (auto it = std::find_if(m_elements.begin(), m_elements.end(), [&node](const Element& element) { return element.node() == node; });
			it != m_elements.end())
		{
			it->setEditorSelected(EditorSelectedYN::Yes);
			unfoldForNode(node);
			m_lastEditorSelectedNode = node;
			m_shiftSelectOriginNode = node;
		}
	}

	bool hasSelection() const
	{
		return std::any_of(m_elements.begin(), m_elements.end(), [](const Element& element) { return element.editorSelected(); });
	}

	void unfoldForNode(const std::shared_ptr<Node>& node)
	{
		if (auto pElement = getElementByNode(node))
		{
			pElement->setFolded(FoldedYN::No);
			if (auto parentNode = node->parent())
			{
				unfoldForNode(parentNode);
			}
		}
	}

	bool canPaste() const
	{
		return !m_copiedNodeJSONs.empty();
	}

	void onClickNewNode()
	{
		// 最後に選択したノードの兄弟として新規ノードを作成
		if (const auto lastEditorSelectedNode = m_lastEditorSelectedNode.lock())
		{
			if (const auto parentNode = lastEditorSelectedNode->parent())
			{
				onClickNewNode(parentNode);
			}
			else
			{
				onClickNewNode(m_canvas->rootNode());
			}
		}
		else
		{
			onClickNewNode(m_canvas->rootNode());
		}
	}

	void onClickNewNode(std::shared_ptr<Node> parentNode)
	{
		if (!parentNode)
		{
			throw Error{ U"Parent node is nullptr" };
		}

		const auto newNode = parentNode->emplaceChild(
			U"Node",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ 100, 100 },
			});
		refreshNodeList();
		selectSingleNode(newNode);
	}

	void onClickDelete()
	{
		for (auto it = m_elements.begin(); it != m_elements.end();)
		{
			if (it->editorSelected())
			{
				if (it->node()->removeFromParent())
				{
					it = m_elements.erase(it);
				}
				else
				{
					++it;
				}
			}
			else
			{
				++it;
			}
		}
		refreshNodeList();
		clearSelection();
	}

	void onClickCut()
	{
		onClickCopy();
		onClickDelete();
	}

	Array<std::shared_ptr<Node>> getSelectedNodesForCopy() const
	{
		// 選択中のノードを列挙
		// ただし、親が選択中の場合は子は含めない
		Array<std::shared_ptr<Node>> selectedNodes;
		for (const auto& element : m_elements)
		{
			if (element.editorSelected())
			{
				bool parentSelected = false;
				for (const auto& parent : selectedNodes)
				{
					if (parent->containsChild(element.node(), RecursiveYN::Yes))
					{
						parentSelected = true;
						break;
					}
				}
				if (!parentSelected)
				{
					selectedNodes.push_back(element.node());
				}
			}
		}
		return selectedNodes;
	}

	void onClickCopy()
	{
		m_copiedNodeJSONs.clear();

		// 選択中のノードをコピー
		const auto selectedNodes = getSelectedNodesForCopy();
		m_copiedNodeJSONs.reserve(selectedNodes.size());
		for (const auto& selectedNode : selectedNodes)
		{
			m_copiedNodeJSONs.push_back(selectedNode->toJSON());
		}
	}

	void onClickDuplicate()
	{
		const auto selectedNodes = getSelectedNodesForCopy();
		if (selectedNodes.empty())
		{
			return;
		}

		// 複製実行
		Array<std::shared_ptr<Node>> newNodes;
		newNodes.reserve(selectedNodes.size());
		for (const auto& selectedNode : selectedNodes)
		{
			const auto parentNode = selectedNode->parent();
			if (!parentNode)
			{
				continue;
			}
			const auto newNode = parentNode->addChildFromJSON(selectedNode->toJSON(), RefreshesLayoutYN::No);
			newNodes.push_back(newNode);
		}
		m_canvas->refreshLayout();
		refreshNodeList();
		selectNodes(newNodes);
	}

	void onClickPaste()
	{
		// 最後に選択したノードの兄弟として貼り付け
		// TODO: 選択したノードの後ろに貼り付けるように
		if (const auto lastEditorSelectedNode = m_lastEditorSelectedNode.lock())
		{
			if (lastEditorSelectedNode->parent())
			{
				onClickPaste(lastEditorSelectedNode->parent());
			}
			else
			{
				onClickPaste(m_canvas->rootNode());
			}
		}
		else
		{
			onClickPaste(m_canvas->rootNode());
		}
	}

	void onClickPaste(std::shared_ptr<Node> parentNode)
	{
		if (!parentNode)
		{
			throw Error{ U"Parent node is nullptr" };
		}

		if (m_copiedNodeJSONs.empty())
		{
			return;
		}

		// 貼り付け実行
		Array<std::shared_ptr<Node>> newNodes;
		for (const auto& copiedNodeJSON : m_copiedNodeJSONs)
		{
			newNodes.push_back(parentNode->addChildFromJSON(copiedNodeJSON, RefreshesLayoutYN::No));
		}
		m_canvas->refreshLayout();
		refreshNodeList();
		selectNodes(newNodes);
	}

	void onClickCreateEmptyParent()
	{
		const auto selectedNode = m_lastEditorSelectedNode.lock();
		if (!selectedNode)
		{
			return;
		}

		auto oldParent = selectedNode->parent();
		if (!oldParent)
		{
			return;
		}

		// selectedNodeが兄弟同士の中で何番目の要素かを調べる
		auto& siblings = oldParent->children();
		auto it = std::find(siblings.begin(), siblings.end(), selectedNode);
		if (it == siblings.end())
		{
			return;
		}
		const size_t idx = std::distance(siblings.begin(), it);

		// 親から切り離す
		selectedNode->removeFromParent();

		// 元ノードと同じインデックスに同じレイアウト設定で空の親ノードを生成
		const auto newParent = Node::Create(U"Node", selectedNode->constraint());
		oldParent->addChildAtIndex(newParent, idx);

		// 新しい親のもとへ子として追加
		newParent->addChild(selectedNode);

		// 元オブジェクトはアンカーがMiddleCenterのAnchorConstraintに変更する
		const RectF originalCalculatedRect = selectedNode->layoutAppliedRect();
		selectedNode->setConstraint(AnchorConstraint
		{
			.anchorMin = Anchor::MiddleCenter,
			.anchorMax = Anchor::MiddleCenter,
			.posDelta = Vec2{ 0, 0 },
			.sizeDelta = originalCalculatedRect.size,
			.pivot = Anchor::MiddleCenter,
		});

		refreshNodeList();
		selectSingleNode(newParent);
	}

	void onClickMoveUp()
	{
		// 選択中のノードを列挙
		Array<std::shared_ptr<Node>> selectedNodes;
		for (const auto& element : m_elements)
		{
			if (element.editorSelected())
			{
				selectedNodes.push_back(element.node());
			}
		}
		if (selectedNodes.isEmpty())
		{
			return;
		}

		// 親ノードごとに処理
		HashTable<std::shared_ptr<Node>, Array<std::shared_ptr<Node>>> selectionByParent;
		selectionByParent.reserve(selectedNodes.size());
		for (const auto& child : selectedNodes)
		{
			if (auto parent = child->parent())
			{
				selectionByParent[parent].push_back(child);
			}
		}
		for (auto& [parent, childrenToMove] : selectionByParent)
		{
			const auto& siblings = parent->children();

			// 選択中の子のインデックスを取得
			Array<size_t> indices;
			indices.reserve(childrenToMove.size());
			for (const auto& child : childrenToMove)
			{
				const auto it = std::find(siblings.begin(), siblings.end(), child);
				if (it != siblings.end())
				{
					indices.push_back(std::distance(siblings.begin(), it));
				}
			}

			// インデックスが小さい順に上の要素と入れ替え
			std::sort(indices.begin(), indices.end());
			for (auto index : indices)
			{
				if (index > 0)
				{
					parent->swapChildren(index, index - 1);
				}
			}
		}
		m_canvas->refreshLayout();
		refreshNodeList();
		selectNodes(selectedNodes);
	}

	void onClickMoveDown()
	{
		// 選択中のノードを列挙
		Array<std::shared_ptr<Node>> selectedNodes;
		for (const auto& element : m_elements)
		{
			if (element.editorSelected())
			{
				selectedNodes.push_back(element.node());
			}
		}
		if (selectedNodes.isEmpty())
		{
			return;
		}

		// 親ノードごとに処理
		HashTable<std::shared_ptr<Node>, Array<std::shared_ptr<Node>>> selectionByParent;
		selectionByParent.reserve(selectedNodes.size());
		for (const auto& child : selectedNodes)
		{
			if (auto parent = child->parent())
			{
				selectionByParent[parent].push_back(child);
			}
		}
		for (auto& [parent, childrenToMove] : selectionByParent)
		{
			const auto& siblings = parent->children();

			// 選択中の子のインデックスを取得
			Array<size_t> indices;
			indices.reserve(childrenToMove.size());
			for (const auto& child : childrenToMove)
			{
				const auto it = std::find(siblings.begin(), siblings.end(), child);
				if (it != siblings.end())
				{
					indices.push_back(std::distance(siblings.begin(), it));
				}
			}

			// インデックスが大きい順に下の要素と入れ替え
			std::sort(indices.begin(), indices.end(), std::greater<size_t>());
			for (auto index : indices)
			{
				if (index < siblings.size() - 1)
				{
					parent->swapChildren(index, index + 1);
				}
			}
		}
		m_canvas->refreshLayout();
		refreshNodeList();
		selectNodes(selectedNodes);
	}

	void clearSelection(bool clearShiftSelectOrigin = true)
	{
		for (auto& element : m_elements)
		{
			element.setEditorSelected(EditorSelectedYN::No);
		}
		if (clearShiftSelectOrigin)
		{
			m_shiftSelectOriginNode.reset();
		}
		m_lastEditorSelectedNode = std::weak_ptr<Node>{};
	}

	void update()
	{
		m_editorHoveredNode.reset();
		for (size_t i = 0; i < m_elements.size(); ++i)
		{
			auto& element = m_elements[i];
			if (element.hierarchyNode()->isHovered())
			{
				m_editorHoveredNode = element.node();
			}

			if (element.node()->isHitTarget())
			{
				const InteractState interactState = element.node()->currentInteractState();
				switch (interactState)
				{
				case InteractState::Default:
					element.elementDetail().hierarchyStateLabel->setText(element.node()->selected() ? U"[Selected]" : U"[Default]");
					break;
				case InteractState::Hovered:
					element.elementDetail().hierarchyStateLabel->setText(element.node()->selected() ? U"[Selected, Hovered]" : U"[Hovered]");
					break;
				case InteractState::Pressed:
					element.elementDetail().hierarchyStateLabel->setText(element.node()->selected() ? U"[Selected, Pressed]" : U"[Pressed]");
					break;
				case InteractState::Disabled:
					element.elementDetail().hierarchyStateLabel->setText(element.node()->selected() ? U"[Selected, Disabled]" : U"[Disabled]");
					break;
				default:
					throw Error{ U"Invalid InteractState: {}"_fmt(static_cast<std::underlying_type_t<InteractState>>(interactState)) };
				}
			}
			else
			{
				element.elementDetail().hierarchyStateLabel->setText(element.node()->selected() ? U"[Selected]" : U"");
			}

			if (element.hierarchyNode()->isClicked())
			{
				if (KeyShift.pressed() && !m_shiftSelectOriginNode.expired())
				{
					const auto originIt = std::find_if(m_elements.begin(), m_elements.end(), [originNode = m_shiftSelectOriginNode.lock()](const auto& e) { return e.node() == originNode; });
					if (originIt == m_elements.end())
					{
						throw Error{ U"Shift select origin node not found in m_elements" };
					}
					clearSelection(false);
					const auto originIndex = static_cast<size_t>(std::distance(m_elements.begin(), originIt));
					const auto start = Min(originIndex, i);
					const auto end = Max(originIndex, i);
					for (size_t j = start; j <= end; ++j)
					{
						m_elements[j].setEditorSelected(EditorSelectedYN::Yes);
					}
				}
				else
				{
					if (KeyControl.pressed())
					{
						// Ctrlキーを押しながらクリックした場合は選択/非選択を切り替え
						const EditorSelectedYN newSelected = EditorSelectedYN{ !element.editorSelected() };
						element.setEditorSelected(newSelected);
						if (newSelected)
						{
							m_shiftSelectOriginNode = element.node();
						}
						else
						{
							m_shiftSelectOriginNode.reset();
						}
					}
					else
					{
						// 普通にクリックした場合は1つだけ選択
						// (複数回押しても選択/非選択の切り替えはしない)
						clearSelection();
						element.setEditorSelected(EditorSelectedYN::Yes);
						m_shiftSelectOriginNode = element.node();
					}
				}

				if (m_elements.count_if([](const auto& e) { return e.editorSelected().getBool(); }) == 1)
				{
					const auto selectedNode = std::find_if(m_elements.begin(), m_elements.end(), [](const auto& e) { return e.editorSelected(); })->node();
					m_lastEditorSelectedNode = selectedNode;
				}
				else
				{
					m_lastEditorSelectedNode = std::weak_ptr<Node>{};
				}
			}

			if (m_hierarchyRootNode->isClicked())
			{
				clearSelection();
			}
		}
	}

	void applyFolding()
	{
		if (m_elements.empty())
		{
			return;
		}
		auto& rootElement = m_elements.front();
		applyFoldingRecursive(rootElement, FoldedYN::No);
	}

	[[nodiscard]]
	const std::weak_ptr<Node>& selectedNode() const
	{
		return m_lastEditorSelectedNode;
	}

	const std::shared_ptr<Node>& hierarchyFrameNode() const
	{
		return m_hierarchyFrameNode;
	}

	void drawSelectedNodesGizmo() const
	{
		const auto editorHoveredNode = m_editorHoveredNode.lock();

		for (const auto& element : m_elements)
		{
			const auto& node = element.node();
			if (!node->activeInHierarchy())
			{
				continue;
			}

			constexpr double Thickness = 2.0;
			const EditorSelectedYN editorSelected = element.editorSelected();
			if (editorSelected)
			{
				node->rect().stretched(Thickness / 2).drawFrame(Thickness, Palette::Orange);

				// 上下左右にリサイズハンドルを表示
				// TODO: リサイズ可能にする
				/*constexpr double HandleSize = 3.0;
				const auto rect = node->rect().stretched(Thickness / 2);
				Circle{ rect.topCenter(), HandleSize }.draw(Palette::Orange);
				Circle{ rect.bottomCenter(), HandleSize }.draw(Palette::Orange);
				Circle{ rect.leftCenter(), HandleSize }.draw(Palette::Orange);
				Circle{ rect.rightCenter(), HandleSize }.draw(Palette::Orange);*/
			}

			if (node == editorHoveredNode)
			{
				const auto& rect = node->rect();
				rect.draw(ColorF{ 1.0, 0.1 });
				if (!editorSelected)
				{
					rect.stretched(Thickness / 2).drawFrame(Thickness, ColorF{ 1.0 });
				}
			}
		}
	}
};

class Inspector
{
private:
	std::shared_ptr<Canvas> m_canvas;
	std::shared_ptr<Canvas> m_editorCanvas;
	std::shared_ptr<Node> m_inspectorFrameNode;
	std::shared_ptr<Node> m_inspectorInnerFrameNode;
	std::shared_ptr<Node> m_inspectorRootNode;
	std::shared_ptr<ContextMenu> m_contextMenu;
	std::weak_ptr<Node> m_targetNode;
	std::function<void()> m_onChangeNodeName;

	template <class TComponent, class... Args>
	void onClickAddComponent(Args&&... args)
	{
		const auto node = m_targetNode.lock();
		if (!node)
		{
			return;
		}
		node->emplaceComponent<TComponent>(std::forward<Args>(args)...);
		refreshInspector();
	}

public:
	Inspector(const std::shared_ptr<Canvas>& canvas, const std::shared_ptr<Canvas>& editorCanvas, const std::shared_ptr<ContextMenu>& contextMenu, std::function<void()> onChangeNodeName)
		: m_canvas(canvas)
		, m_editorCanvas(editorCanvas)
		, m_inspectorFrameNode(editorCanvas->rootNode()->emplaceChild(
			U"InspectorFrame",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopRight,
				.anchorMax = Anchor::BottomRight,
				.posDelta = Vec2{ 0, MenuBarHeight },
				.sizeDelta = Vec2{ 400, -MenuBarHeight },
				.pivot = Anchor::TopRight,
			}))
		, m_inspectorInnerFrameNode(m_inspectorFrameNode->emplaceChild(
			U"InspectorInnerFrame",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::BottomRight,
				.posDelta = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ -2, -2 },
				.pivot = Anchor::MiddleCenter,
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered))
		, m_inspectorRootNode(m_inspectorInnerFrameNode->emplaceChild(
			U"Inspector",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopLeft,
				.anchorMax = Anchor::BottomRight,
				.posDelta = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ -10, -10 },
				.pivot = Anchor::MiddleCenter,
			},
			IsHitTargetYN::No))
		, m_contextMenu(contextMenu)
		, m_onChangeNodeName(std::move(onChangeNodeName))
	{
		m_inspectorFrameNode->emplaceComponent<RectRenderer>(ColorF{ 0.5, 0.4 }, Palette::Black, 0.0, 10.0);
		m_inspectorInnerFrameNode->emplaceComponent<RectRenderer>(ColorF{ 0.1, 0.8 }, Palette::Black, 0.0, 10.0);
		m_inspectorInnerFrameNode->emplaceComponent<ContextMenuOpener>(contextMenu,
			Array<MenuElement>
			{
				MenuItem{ U"Sprite を追加", U"", [this] { onClickAddComponent<Sprite>(); } },
				MenuItem{ U"RectRenderer を追加", U"", [this] { onClickAddComponent<RectRenderer>(); } },
				MenuItem{ U"TextBox を追加", U"", [this] { onClickAddComponent<TextBox>(); } },
				MenuItem{ U"Label を追加", U"", [this] { onClickAddComponent<Label>(); } },
			});
		m_inspectorRootNode->setLayout(VerticalLayout{ .padding = LRTB{ 0, 0, 4, 4 } });
		m_inspectorRootNode->setVerticalScrollable(true);
	}

	void refreshInspector()
	{
		setTargetNode(m_targetNode.lock());
		m_editorCanvas->refreshLayout();
	}

	void setTargetNode(const std::shared_ptr<Node>& targetNode)
	{
		m_targetNode = targetNode;

		m_inspectorRootNode->removeChildrenAll();

		if (targetNode)
		{
			const auto nodeNameNode = createNodeNameNode(targetNode);
			m_inspectorRootNode->addChild(nodeNameNode);

			const auto nodeSettingNode = createNodeSettingNode(targetNode);
			m_inspectorRootNode->addChild(nodeSettingNode);

			const auto constraintNode = createConstraintNode(targetNode);
			m_inspectorRootNode->addChild(constraintNode);

			const auto layoutNode = createLayoutNode(targetNode);
			m_inspectorRootNode->addChild(layoutNode);

			const auto transformEffectNode = createTransformEffectNode(&targetNode->transformEffect());
			m_inspectorRootNode->addChild(transformEffectNode);

			for (const auto& component : targetNode->components())
			{
				const auto componentNode = createComponentNode(targetNode, component);
				m_inspectorRootNode->addChild(componentNode);
			}
		}
	}

	[[nodiscard]]
	static std::shared_ptr<Node> CreateHeadingNode(StringView name, const ColorF& color)
	{
		auto headingNode = Node::Create(U"Heading", BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 24 },
				.margin = LRTB{ 0, 0, 0, 0 },
			});
		headingNode->emplaceComponent<RectRenderer>(
			PropertyValue<ColorF>(ColorF{ color, 0.8 }).withHover(ColorF{ color + ColorF{ 0.05 }, 0.8 }).withPressed(ColorF{ color - ColorF{ 0.05 }, 0.8 }),
			Palette::Black,
			0.0,
			3.0);
		const auto arrowLabel = headingNode->emplaceComponent<Label>(
			U"▼",
			U"Font14Bold",
			14,
			ColorF{ 1.0, 0.6 },
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 5, 5, 0, 0 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip);
		headingNode->emplaceComponent<Label>(
			name,
			U"Font14Bold",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 25, 5, 0, 0 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip);
		headingNode->addOnClick([arrowLabel](const std::shared_ptr<Node>& node)
			{
				if (const auto parent = node->parent())
				{
					bool isFolded = false;

					// 見出し以外のアクティブを入れ替え
					for (const auto child : parent->children())
					{
						if (child != node)
						{
							child->setActive(!child->activeSelf());
							if (!child->activeSelf())
							{
								isFolded = true;
							}
						}
					}

					// 矢印を回転
					arrowLabel->setText(isFolded ? U"▶" : U"▼");

					// 折り畳み時はpaddingを付けない
					auto layout = parent->layout();
					if (auto pVerticalLayout = std::get_if<VerticalLayout>(&layout))
					{
						pVerticalLayout->padding = isFolded ? LRTB::Zero() : LRTB{ 0, 0, 0, 8 };
					}
					parent->setLayout(layout, RefreshesLayoutYN::No);

					// 高さをフィットさせる
					parent->setBoxConstraintToFitToChildren(FitTarget::HeightOnly, RefreshesLayoutYN::Yes);
				}
			});

		return headingNode;
	}

	class PropertyTextBox : public ComponentBase
	{
	private:
		std::shared_ptr<TextBox> m_textBox;
		std::function<void(StringView)> m_fnSetValue;

		void update(CanvasUpdateContext*, const std::shared_ptr<Node>&) override
		{
			if (m_textBox->isChanged())
			{
				m_fnSetValue(m_textBox->text());
			}
		}

		void draw(const Node&) const override
		{
		}

	public:
		explicit PropertyTextBox(const std::shared_ptr<TextBox>& textBox, std::function<void(StringView)> fnSetValue)
			: ComponentBase{ U"PropertyTextBoxUpdater", {} }
			, m_textBox(textBox)
			, m_fnSetValue(std::move(fnSetValue))
		{
		}
	};

	[[nodiscard]]
	static std::shared_ptr<Node> CreateNodeNameTextboxNode(StringView name, StringView value, std::function<void(StringView)> fnSetValue)
	{
		const auto propertyNode = Node::Create(name, BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ -24, 32 },
			},
			IsHitTargetYN::Yes);
		const auto textBoxNode = propertyNode->emplaceChild(
			U"TextBox",
			AnchorConstraint
			{
				.anchorMin = Anchor::MiddleLeft,
				.anchorMax = Anchor::MiddleRight,
				.posDelta = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ -16, 26 },
				.pivot = Anchor::MiddleCenter,
			});
		textBoxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.5, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBox = textBoxNode->emplaceComponent<TextBox>(U"Font14", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBox->setText(value, IgnoreIsChangedYN::Yes);
		textBoxNode->addComponent(std::make_shared<PropertyTextBox>(textBox, std::move(fnSetValue)));
		return propertyNode;
	}

	[[nodiscard]]
	static std::shared_ptr<Node> CreatePropertyNode(StringView name, StringView value, std::function<void(StringView)> fnSetValue)
	{
		const auto propertyNode = Node::Create(name, BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 32 },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered);
		propertyNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHover(ColorF{ 1.0, 0.1 }), Palette::Black, 0.0, 3.0);
		propertyNode->emplaceComponent<Label>(
			name,
			U"Font14",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Top,
			LRTB{ 15, 5, 5, 5 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip);
		const auto textBoxNode = propertyNode->emplaceChild(
			U"TextBox",
			AnchorConstraint
			{
				.anchorMin = Anchor::MiddleRight,
				.anchorMax = Anchor::MiddleRight,
				.posDelta = Vec2{ -8, 0 },
				.sizeDelta = Vec2{ 200, 26 },
				.pivot = Anchor::MiddleRight,
			});
		textBoxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.5, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBox = textBoxNode->emplaceComponent<TextBox>(U"Font14", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBox->setText(value, IgnoreIsChangedYN::Yes);
		textBoxNode->addComponent(std::make_shared<PropertyTextBox>(textBox, std::move(fnSetValue)));
		return propertyNode;
	}

	[[nodiscard]]
	static std::shared_ptr<Node> CreateVec2PropertyNode(
		StringView name,
		const Vec2& currentValue,
		std::function<void(const Vec2&)> fnSetValue)
	{
		const auto propertyNode = Node::Create(
			name,
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 32 },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered);

		propertyNode->emplaceComponent<RectRenderer>(
			PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHover(ColorF{ 1.0, 0.1 }),
			Palette::Black,
			0.0,
			3.0);

		propertyNode->emplaceComponent<Label>(
			name,
			U"Font14",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Top,
			LRTB{ 15, 5, 5, 5 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip);

		const auto textBoxParentNode = propertyNode->emplaceChild(
			U"TextBoxParent",
			AnchorConstraint
			{
				.anchorMin = Anchor::MiddleRight,
				.anchorMax = Anchor::MiddleRight,
				.posDelta = Vec2{ -8, 0 },
				.sizeDelta = Vec2{ 200, 26 },
				.pivot = Anchor::MiddleRight,
			},
			IsHitTargetYN::No,
			InheritChildrenStateFlags::Hovered);

		// X
		const auto textBoxXNode = textBoxParentNode->emplaceChild(
			U"TextBoxX",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0.5, 1 },
				.sizeDelta = Vec2{ -2, 0 },
				.margin = LRTB{ 0, 2, 0, 0 },
			});
		textBoxXNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.5, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxX = textBoxXNode->emplaceComponent<TextBox>(
			U"Font14", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxX->setText(Format(currentValue.x), IgnoreIsChangedYN::Yes);

		// Y
		const auto textBoxYNode = textBoxParentNode->emplaceChild(
			U"TextBoxY",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0.5, 1 },
				.sizeDelta = Vec2{ -2, 0 },
				.margin = LRTB{ 2, 0, 0, 0 },
			});
		textBoxYNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.5, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxY = textBoxYNode->emplaceComponent<TextBox>(
			U"Font14", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxY->setText(Format(currentValue.y), IgnoreIsChangedYN::Yes);

		class Vec2PropertyTextBox : public ComponentBase
		{
		private:
			std::shared_ptr<TextBox> m_textBoxX;
			std::shared_ptr<TextBox> m_textBoxY;
			std::function<void(const Vec2&)> m_fnSetValue;
			Vec2 m_prevValue;

		public:
			Vec2PropertyTextBox(
				const std::shared_ptr<TextBox>& textBoxX,
				const std::shared_ptr<TextBox>& textBoxY,
				std::function<void(const Vec2&)> fnSetValue,
				const Vec2& initialValue)
				: ComponentBase{ U"Vec2PropertyTextBox", {} }
				, m_textBoxX(textBoxX)
				, m_textBoxY(textBoxY)
				, m_fnSetValue(std::move(fnSetValue))
				, m_prevValue(initialValue)
			{
			}

			void update(CanvasUpdateContext*, const std::shared_ptr<Node>&) override
			{
				const double x = ParseOpt<double>(m_textBoxX->text()).value_or(m_prevValue.x);
				const double y = ParseOpt<double>(m_textBoxY->text()).value_or(m_prevValue.y);

				const Vec2 newValue{ x, y };
				if (newValue != m_prevValue)
				{
					m_prevValue = newValue;
					if (m_fnSetValue)
					{
						m_fnSetValue(newValue);
					}
				}
			}

			void draw(const Node&) const override
			{
			}
		};

		propertyNode->addComponent(std::make_shared<Vec2PropertyTextBox>(
			textBoxX,
			textBoxY,
			fnSetValue,
			currentValue
		));

		return propertyNode;
	}

	[[nodiscard]]
	static std::shared_ptr<Node> CreateVec4PropertyNode(
		StringView name,
		const Vec4& currentValue,
		std::function<void(const Vec4&)> fnSetValue)
	{
		const auto propertyNode = Node::Create(
			name,
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 32 },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered);

		propertyNode->emplaceComponent<RectRenderer>(
			PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHover(ColorF{ 1.0, 0.1 }),
			Palette::Black,
			0.0,
			3.0);

		propertyNode->emplaceComponent<Label>(
			name,
			U"Font14",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Top,
			LRTB{ 15, 5, 5, 5 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip);

		const auto textBoxParentNode = propertyNode->emplaceChild(
			U"TextBoxParent",
			AnchorConstraint
			{
				.anchorMin = Anchor::MiddleRight,
				.anchorMax = Anchor::MiddleRight,
				.posDelta = Vec2{ -8, 0 },
				.sizeDelta = Vec2{ 200, 26 },
				.pivot = Anchor::MiddleRight,
			},
			IsHitTargetYN::No,
			InheritChildrenStateFlags::Hovered);

		// X
		const auto textBoxXNode = textBoxParentNode->emplaceChild(
			U"TextBoxX",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0.25, 1 },
				.sizeDelta = Vec2{ -2, 0 },
				.margin = LRTB{ 0, 2, 0, 0 },
			});
		textBoxXNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.5, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxX = textBoxXNode->emplaceComponent<TextBox>(
			U"Font14", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxX->setText(Format(currentValue.x), IgnoreIsChangedYN::Yes);

		// Y
		const auto textBoxYNode = textBoxParentNode->emplaceChild(
			U"TextBoxY",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0.25, 1 },
				.sizeDelta = Vec2{ -2, 0 },
				.margin = LRTB{ 2, 0, 0, 0 },
			});
		textBoxYNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.5, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxY = textBoxYNode->emplaceComponent<TextBox>(
			U"Font14", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxY->setText(Format(currentValue.y), IgnoreIsChangedYN::Yes);

		// Z
		const auto textBoxZNode = textBoxParentNode->emplaceChild(
			U"TextBoxZ",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0.25, 1 },
				.sizeDelta = Vec2{ -2, 0 },
				.margin = LRTB{ 2, 0, 0, 0 },
			});
		textBoxZNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.5, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxZ = textBoxZNode->emplaceComponent<TextBox>(
			U"Font14", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxZ->setText(Format(currentValue.z), IgnoreIsChangedYN::Yes);

		// W
		const auto textBoxWNode = textBoxParentNode->emplaceChild(
			U"TextBoxW",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0.25, 1 },
				.sizeDelta = Vec2{ -2, 0 },
				.margin = LRTB{ 2, 0, 0, 0 },
			});
		textBoxWNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.5, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxW = textBoxWNode->emplaceComponent<TextBox>(
			U"Font14", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxW->setText(Format(currentValue.w), IgnoreIsChangedYN::Yes);

		class Vec4PropertyTextBox : public ComponentBase
		{
		private:
			std::shared_ptr<TextBox> m_textBoxX;
			std::shared_ptr<TextBox> m_textBoxY;
			std::shared_ptr<TextBox> m_textBoxZ;
			std::shared_ptr<TextBox> m_textBoxW;
			std::function<void(const Vec4&)> m_fnSetValue;
			Vec4 m_prevValue;

		public:
			Vec4PropertyTextBox(
				const std::shared_ptr<TextBox>& textBoxX,
				const std::shared_ptr<TextBox>& textBoxY,
				const std::shared_ptr<TextBox>& textBoxZ,
				const std::shared_ptr<TextBox>& textBoxW,
				std::function<void(const Vec4&)> fnSetValue,
				const Vec4& initialValue)
				: ComponentBase{ U"Vec4PropertyTextBox", {} }
				, m_textBoxX(textBoxX)
				, m_textBoxY(textBoxY)
				, m_textBoxZ(textBoxZ)
				, m_textBoxW(textBoxW)
				, m_fnSetValue(std::move(fnSetValue))
				, m_prevValue(initialValue)
			{
			}

			void update(CanvasUpdateContext*, const std::shared_ptr<Node>&) override
			{
				const double x = ParseOpt<double>(m_textBoxX->text()).value_or(m_prevValue.x);
				const double y = ParseOpt<double>(m_textBoxY->text()).value_or(m_prevValue.y);
				const double z = ParseOpt<double>(m_textBoxZ->text()).value_or(m_prevValue.z);
				const double w = ParseOpt<double>(m_textBoxW->text()).value_or(m_prevValue.w);

				const Vec4 newValue{ x, y, z, w };
				if (newValue != m_prevValue)
				{
					m_prevValue = newValue;
					if (m_fnSetValue)
					{
						m_fnSetValue(newValue);
					}
				}
			}

			void draw(const Node&) const override
			{
			}
		};

		propertyNode->addComponent(std::make_shared<Vec4PropertyTextBox>(
			textBoxX,
			textBoxY,
			textBoxZ,
			textBoxW,
			fnSetValue,
			currentValue));

		return propertyNode;
	}

	[[nodiscard]]
	static std::shared_ptr<Node> CreateLRTBPropertyNode(
		StringView name,
		const LRTB& currentValue,
		std::function<void(const LRTB&)> fnSetValue)
	{
		constexpr int32 LineHeight = 32;
		const auto propertyNode = Node::Create(
			name,
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, LineHeight * 2 },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered);
		propertyNode->emplaceComponent<RectRenderer>(
			PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHover(ColorF{ 1.0, 0.1 }),
			Palette::Black,
			0.0,
			3.0);
		propertyNode->emplaceComponent<Label>(
			U"{} (left, right)"_fmt(name),
			U"Font14",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Top,
			LRTB{ 15, 5, 5, 5 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip);
		propertyNode->emplaceComponent<Label>(
			U"{} (top, bottom)"_fmt(name),
			U"Font14",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Top,
			LRTB{ 15, 5, 5 + LineHeight, 5 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip);
		const auto textBoxParentNode = propertyNode->emplaceChild(
			U"TextBoxParent",
			AnchorConstraint
			{
				.anchorMin = Anchor::TopRight,
				.anchorMax = Anchor::BottomRight,
				.posDelta = Vec2{ -8, 0 },
				.sizeDelta = Vec2{ 200, -6 },
				.pivot = Anchor::MiddleRight,
			},
			IsHitTargetYN::No,
			InheritChildrenStateFlags::Hovered);

		// L
		const auto textBoxLNode = textBoxParentNode->emplaceChild(
			U"TextBoxL",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0.5, 0 },
				.sizeDelta = Vec2{ -2, 26 },
				.margin = LRTB{ 0, 2, 0, 6 },
			});
		textBoxLNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.5, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxL = textBoxLNode->emplaceComponent<TextBox>(
			U"Font14", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxL->setText(Format(currentValue.left), IgnoreIsChangedYN::Yes);

		// R
		const auto textBoxRNode = textBoxParentNode->emplaceChild(
			U"TextBoxR",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0.5, 0 },
				.sizeDelta = Vec2{ -2, 26 },
				.margin = LRTB{ 0, 2, 0, 6 },
			});
		textBoxRNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.5, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxR = textBoxRNode->emplaceComponent<TextBox>(
			U"Font14", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxR->setText(Format(currentValue.right), IgnoreIsChangedYN::Yes);

		// T
		const auto textBoxTNode = textBoxParentNode->emplaceChild(
			U"TextBoxT",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0.5, 0 },
				.sizeDelta = Vec2{ -2, 26 },
				.margin = LRTB{ 0, 2, 0, 0 },
			});
		textBoxTNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.5, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxT = textBoxTNode->emplaceComponent<TextBox>(
			U"Font14", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxT->setText(Format(currentValue.top), IgnoreIsChangedYN::Yes);

		// B
		const auto textBoxBNode = textBoxParentNode->emplaceChild(
			U"TextBoxB",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0.5, 0 },
				.sizeDelta = Vec2{ -2, 26 },
				.margin = LRTB{ 0, 2, 0, 0 },
			});
		textBoxBNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.5, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxB = textBoxBNode->emplaceComponent<TextBox>(
			U"Font14", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxB->setText(Format(currentValue.bottom), IgnoreIsChangedYN::Yes);

		class LRTBPropertyTextBox : public ComponentBase
		{
		private:
			std::shared_ptr<TextBox> m_textBoxL;
			std::shared_ptr<TextBox> m_textBoxR;
			std::shared_ptr<TextBox> m_textBoxT;
			std::shared_ptr<TextBox> m_textBoxB;
			std::function<void(const LRTB&)> m_fnSetValue;
			LRTB m_prevValue;

		public:
			LRTBPropertyTextBox(
				const std::shared_ptr<TextBox>& textBoxL,
				const std::shared_ptr<TextBox>& textBoxR,
				const std::shared_ptr<TextBox>& textBoxT,
				const std::shared_ptr<TextBox>& textBoxB,
				std::function<void(const LRTB&)> fnSetValue,
				const LRTB& initialValue)
				: ComponentBase{ U"LRTBPropertyTextBox", {} }
				, m_textBoxL(textBoxL)
				, m_textBoxR(textBoxR)
				, m_textBoxT(textBoxT)
				, m_textBoxB(textBoxB)
				, m_fnSetValue(std::move(fnSetValue))
				, m_prevValue(initialValue)
			{
			}
			void update(CanvasUpdateContext*, const std::shared_ptr<Node>&) override
			{
				const double l = ParseOpt<double>(m_textBoxL->text()).value_or(m_prevValue.left);
				const double r = ParseOpt<double>(m_textBoxR->text()).value_or(m_prevValue.right);
				const double t = ParseOpt<double>(m_textBoxT->text()).value_or(m_prevValue.top);
				const double b = ParseOpt<double>(m_textBoxB->text()).value_or(m_prevValue.bottom);
				const LRTB newValue{ l, r, t, b };
				if (newValue != m_prevValue)
				{
					m_prevValue = newValue;
					if (m_fnSetValue)
					{
						m_fnSetValue(newValue);
					}
				}
			}
			void draw(const Node&) const override
			{
			}
		};

		propertyNode->addComponent(std::make_shared<LRTBPropertyTextBox>(
			textBoxL,
			textBoxR,
			textBoxT,
			textBoxB,
			fnSetValue,
			currentValue));

		return propertyNode;
	}

	[[nodiscard]]
	std::shared_ptr<Node> CreateColorPropertyNode(
		StringView name,
		const ColorF& currentValue,
		std::function<void(const ColorF&)> fnSetValue)
	{
		const auto propertyNode = Node::Create(
			name,
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 36 },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered);
		propertyNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHover(ColorF{ 1.0, 0.1 }), Palette::Black, 0.0, 3.0);
		propertyNode->emplaceComponent<Label>(
			name,
			U"Font14",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 15, 5, 5, 5 });

		const auto rowNode = propertyNode->emplaceChild(
			U"ColorPropertyRow",
			AnchorConstraint
			{
				.anchorMin = Anchor::MiddleRight,
				.anchorMax = Anchor::MiddleRight,
				.posDelta = Vec2{ -8, 0 },
				.sizeDelta = Vec2{ 200, 26 },
				.pivot = Anchor::MiddleRight,
			},
			IsHitTargetYN::No,
			InheritChildrenStateFlags::Hovered);

		const auto previewRootNode = rowNode->emplaceChild(
			U"ColorPreviewRoot",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.sizeDelta = Vec2{ 26, 0 },
				.margin = LRTB{ 0, 2, 0, 0 },
			},
			IsHitTargetYN::No);

		// 透明の市松模様
		constexpr int32 GridSize = 3;
		for (int32 y = 0; y < GridSize; ++y)
		{
			for (int32 x = 0; x < GridSize; ++x)
			{
				const bool isOdd = (x + y) % 2 == 1;
				const auto previewNode = previewRootNode->emplaceChild(
					U"Transparent",
					AnchorConstraint
					{
						.anchorMin = { static_cast<double>(x) / GridSize, static_cast<double>(y) / GridSize },
						.anchorMax = { static_cast<double>(x + 1) / GridSize, static_cast<double>(y + 1) / GridSize },
						.pivot = Anchor::TopLeft,
					},
					IsHitTargetYN::No)
					->emplaceComponent<RectRenderer>(ColorF{ isOdd ? 0.9 : 1.0 });
			}
		}

		// 色プレビュー
		const auto previewNode = previewRootNode->emplaceChild(
			U"ColorPreview",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 1 },
				.sizeDelta = Vec2{ 26, 0 },
				.margin = LRTB{ 0, 2, 0, 0 },
			},
			IsHitTargetYN::No);
		const auto previewRectRenderer = previewNode->emplaceComponent<RectRenderer>(currentValue, ColorF{ 1.0, 0.3 }, 1.0, 0.0);

		const auto textBoxParentNode = rowNode->emplaceChild(
			U"TextBoxParent",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 1 },
				.sizeDelta = Vec2{ -28, 0 },
				.margin = LRTB{ 0, 0, 0, 0 },
			},
			IsHitTargetYN::No,
			InheritChildrenStateFlags::Hovered);

		// R
		const auto textBoxRNode = textBoxParentNode->emplaceChild(
			U"TextBoxR",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0.25, 1 },
				.sizeDelta = Vec2{ -2, 0 },
				.margin = LRTB{ 2, 0, 0, 0 },
			});
		textBoxRNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.5, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxR = textBoxRNode->emplaceComponent<TextBox>(
			U"Font14", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxR->setText(Format(currentValue.r), IgnoreIsChangedYN::Yes);

		// G
		const auto textBoxGNode = textBoxParentNode->emplaceChild(
			U"TextBoxG",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0.25, 1 },
				.sizeDelta = Vec2{ -2, 0 },
				.margin = LRTB{ 2, 0, 0, 0 },
			});
		textBoxGNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.5, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxG = textBoxGNode->emplaceComponent<TextBox>(
			U"Font14", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxG->setText(Format(currentValue.g), IgnoreIsChangedYN::Yes);

		// B
		const auto textBoxBNode = textBoxParentNode->emplaceChild(
			U"TextBoxB",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0.25, 1 },
				.sizeDelta = Vec2{ -2, 0 },
				.margin = LRTB{ 2, 0, 0, 0 },
			});
		textBoxBNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.5, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxB = textBoxBNode->emplaceComponent<TextBox>(
			U"Font14", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxB->setText(Format(currentValue.b), IgnoreIsChangedYN::Yes);

		// A
		const auto textBoxANode = textBoxParentNode->emplaceChild(
			U"TextBoxA",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0.25, 1 },
				.sizeDelta = Vec2{ -2, 0 },
				.margin = LRTB{ 2, 0, 0, 0 },
			});
		textBoxANode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.5, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);
		const auto textBoxA = textBoxANode->emplaceComponent<TextBox>(
			U"Font14", 14, Palette::White, Vec2{ 4, 4 }, Vec2{ 2, 2 }, Palette::White, ColorF{ Palette::Orange, 0.5 });
		textBoxA->setText(Format(currentValue.a), IgnoreIsChangedYN::Yes);

		class ColorPropertyTextBox : public ComponentBase
		{
		private:
			std::shared_ptr<TextBox> m_textBoxR;
			std::shared_ptr<TextBox> m_textBoxG;
			std::shared_ptr<TextBox> m_textBoxB;
			std::shared_ptr<TextBox> m_textBoxA;
			std::shared_ptr<RectRenderer> m_previewRect;
			std::function<void(const ColorF&)> m_fnSetValue;

			// 前回の色
			ColorF m_prevColor;

		public:
			ColorPropertyTextBox(
				const std::shared_ptr<TextBox>& r,
				const std::shared_ptr<TextBox>& g,
				const std::shared_ptr<TextBox>& b,
				const std::shared_ptr<TextBox>& a,
				const std::shared_ptr<RectRenderer>& previewRect,
				std::function<void(const ColorF&)> fnSetValue,
				const ColorF& initialColor)
				: ComponentBase{ U"ColorPropertyTextBox", {} }
				, m_textBoxR(r)
				, m_textBoxG(g)
				, m_textBoxB(b)
				, m_textBoxA(a)
				, m_previewRect(previewRect)
				, m_fnSetValue(std::move(fnSetValue))
				, m_prevColor(initialColor)
			{
			}

			void update(CanvasUpdateContext*, const std::shared_ptr<Node>&) override
			{
				const double r = Clamp(ParseOpt<double>(m_textBoxR->text()).value_or(m_prevColor.r), 0.0, 1.0);
				const double g = Clamp(ParseOpt<double>(m_textBoxG->text()).value_or(m_prevColor.g), 0.0, 1.0);
				const double b = Clamp(ParseOpt<double>(m_textBoxB->text()).value_or(m_prevColor.b), 0.0, 1.0);
				const double a = Clamp(ParseOpt<double>(m_textBoxA->text()).value_or(m_prevColor.a), 0.0, 1.0);

				const ColorF newColor{ r, g, b, a };
				if (newColor != m_prevColor)
				{
					m_prevColor = newColor;
					if (m_fnSetValue)
					{
						m_fnSetValue(newColor);
					}
					m_previewRect->setFillColor(newColor);
				}
			}
		};

		propertyNode->addComponent(std::make_shared<ColorPropertyTextBox>(
			textBoxR,
			textBoxG,
			textBoxB,
			textBoxA,
			previewRectRenderer,
			fnSetValue,
			currentValue));

		return propertyNode;
	}

	[[nodiscard]]
	static std::shared_ptr<Node> CreateEnumPropertyNode(
		StringView name,
		StringView currentValue,
		std::function<void(StringView)> fnSetValue,
		const std::shared_ptr<ContextMenu>& contextMenu,
		const Array<String>& enumCandidates)
	{
		const auto propertyNode = Node::Create(
			name,
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 32 },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered);

		propertyNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHover(ColorF{ 1.0, 0.1 }), Palette::Black, 0.0, 3.0);

		propertyNode->emplaceComponent<Label>(
			name,
			U"Font14",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 15, 5, 5, 5 });

		const auto comboBoxNode = propertyNode->emplaceChild(
			U"ComboBox",
			AnchorConstraint
			{
				.anchorMin = Anchor::MiddleRight,
				.anchorMax = Anchor::MiddleRight,
				.posDelta = Vec2{ -8, 0 },
				.sizeDelta = Vec2{ 200, 26 },
				.pivot = Anchor::MiddleRight,
			});

		comboBoxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.5, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHover(ColorF{ 1.0, 0.6 }).withSmoothTime(0.05), 1.0, 4.0);

		const auto enumLabel = comboBoxNode->emplaceComponent<Label>(
			currentValue,
			U"Font14",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Middle,
			LRTB{ 5, 5, 5, 5 });

		class EnumPropertyComboBox : public ComponentBase
		{
		private:
			String m_currentValue;
			std::function<void(StringView)> m_onSetValue;
			std::shared_ptr<Label> m_label;
			std::shared_ptr<ContextMenu> m_contextMenu;
			Array<String> m_enumCandidates;

		public:
			EnumPropertyComboBox(
				StringView initialValue,
				std::function<void(StringView)> onSetValue,
				const std::shared_ptr<Label>& label,
				const std::shared_ptr<ContextMenu>& contextMenu,
				const Array<String>& enumCandidates)
				: ComponentBase{ U"EnumPropertyComboBox", {} }
				, m_currentValue(initialValue)
				, m_onSetValue(std::move(onSetValue))
				, m_label(label)
				, m_contextMenu(contextMenu)
				, m_enumCandidates(enumCandidates)
			{
			}

			void update(CanvasUpdateContext*, const std::shared_ptr<Node>& node) override
			{
				if (node->isClicked())
				{
					Array<MenuElement> menuElements;
					for (const auto& name : m_enumCandidates)
					{
						menuElements.push_back(
							MenuItem
							{
								name,
								U"",
								[this, name]
								{
									m_currentValue = name;
									m_label->setText(name);
									m_onSetValue(m_currentValue);
								}
							});
					}
					m_contextMenu->show(node->rect().bl(), menuElements);
				}
			}
		};

		comboBoxNode->addComponent(std::make_shared<EnumPropertyComboBox>(
			currentValue,
			fnSetValue,
			enumLabel,
			contextMenu,
			enumCandidates));

		comboBoxNode->emplaceComponent<Label>(
			U"▼",
			U"Font14",
			10,
			Palette::White,
			HorizontalAlign::Right,
			VerticalAlign::Middle,
			LRTB{ 5, 7, 5, 5 });

		return propertyNode;
	}

	[[nodiscard]]
	static std::shared_ptr<Node> CreateCheckboxNode(
		bool initialValue,
		std::function<void(bool)> fnSetValue,
		bool useParentHoverState = false)
	{
		auto checkboxNode = Node::Create(
			U"Checkbox",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ 18, 18 },
			},
			useParentHoverState ? IsHitTargetYN::No : IsHitTargetYN::Yes);

		checkboxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.1, 0.8 } }.withDisabled(ColorF{ 0.5, 0.8 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ 1.0, 0.4 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 1.0, 4.0);

		auto checkLabel = checkboxNode->emplaceComponent<Label>(
			initialValue ? U"✓" : U"",
			U"Font14",
			14,
			Palette::White,
			HorizontalAlign::Center,
			VerticalAlign::Middle);

		class CheckboxToggler : public ComponentBase
		{
		private:
			bool m_value;
			std::function<void(bool)> m_fnSetValue;
			std::shared_ptr<Label> m_checkLabel;
			bool m_useParentHoverState;

		public:
			CheckboxToggler(bool initialValue,
				std::function<void(bool)> fnSetValue,
				const std::shared_ptr<Label>& checkLabel,
				bool useParentHoverState)
				: ComponentBase(U"CheckboxToggler", {})
				, m_value(initialValue)
				, m_fnSetValue(std::move(fnSetValue))
				, m_checkLabel(checkLabel)
				, m_useParentHoverState(useParentHoverState)
			{
			}

			void update(CanvasUpdateContext*, const std::shared_ptr<Node>& node) override
			{
				// クリックでON/OFFをトグル
				bool isClicked = false;
				if (m_useParentHoverState)
				{
					if (const auto parent = node->parent())
					{
						isClicked = parent->isClicked();
					}
				}
				else
				{
					isClicked = node->isClicked();
				}
				if (isClicked)
				{
					m_value = !m_value;
					m_checkLabel->setText(m_value ? U"✓" : U"");
					if (m_fnSetValue)
					{
						m_fnSetValue(m_value);
					}
				}
			}
		};

		checkboxNode->addComponent(std::make_shared<CheckboxToggler>(
			initialValue,
			std::move(fnSetValue),
			checkLabel,
			useParentHoverState));

		return checkboxNode;
	}

	[[nodiscard]]
	static std::shared_ptr<Node> CreateBoolPropertyNode(
		StringView name,
		bool currentValue,
		std::function<void(bool)> fnSetValue)
	{
		auto propertyNode = Node::Create(
			name,
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 32 },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::Hovered);
		propertyNode->emplaceComponent<RectRenderer>(
			PropertyValue<ColorF>(ColorF{ 1.0, 0.0 }).withHover(ColorF{ 1.0, 0.1 }),
			Palette::Black,
			0.0,
			3.0);

		propertyNode->emplaceComponent<Label>(
			name,
			U"Font14",
			14,
			Palette::White,
			HorizontalAlign::Left,
			VerticalAlign::Top,
			LRTB{ 15, 5, 5, 5 },
			HorizontalOverflow::Wrap,
			VerticalOverflow::Clip);

		const auto checkboxNode = CreateCheckboxNode(currentValue, fnSetValue, true);
		checkboxNode->setConstraint(
			AnchorConstraint
			{
				.anchorMin = Anchor::MiddleRight,
				.anchorMax = Anchor::MiddleRight,
				.posDelta = Vec2{ -12, 0 },
				.sizeDelta = Vec2{ 18, 18 },
				.pivot = Anchor::MiddleRight,
			});
		propertyNode->addChild(checkboxNode);

		return propertyNode;
	}

	[[nodiscard]]
	std::shared_ptr<Node> createNodeNameNode(const std::shared_ptr<Node>& node)
	{
		const auto nodeNameNode = Node::Create(
			U"NodeName",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.sizeDelta = Vec2{ 0, 40 },
				.margin = LRTB{ 0, 0, 0, 8 },
			});
		nodeNameNode->setLayout(HorizontalLayout{ .padding = 6 });
		nodeNameNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

		nodeNameNode->addChild(CreateCheckboxNode(node->activeSelf().getBool(), [node](bool value) { node->setActive(value); }));
		nodeNameNode->addChild(CreateNodeNameTextboxNode(U"name", node->name(),
			[this, node](StringView value)
			{
				if (value.empty())
				{
					node->setName(U"Node");
				}
				else
				{
					node->setName(value);
				}
				m_onChangeNodeName();
			}));

		return nodeNameNode;
	}

	[[nodiscard]]
	std::shared_ptr<Node> createNodeSettingNode(const std::shared_ptr<Node>& node)
	{
		auto nodeSettingNode = Node::Create(
			U"NodeSetting",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.margin = LRTB{ 0, 0, 0, 8 },
			});
		nodeSettingNode->setLayout(VerticalLayout{ .padding = LRTB{ 0, 0, 0, 8 } });
		nodeSettingNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

		nodeSettingNode->addChild(CreateHeadingNode(U"Node", ColorF{ 0.5, 0.3, 0.3 }));

		nodeSettingNode->addChild(
			Node::Create(
				U"TopPadding",
				BoxConstraint
				{
					.sizeRatio = Vec2{ 1, 0 },
					.sizeDelta = Vec2{ 0, 8 },
				}));

		const auto fnAddBoolChild =
			[this, &nodeSettingNode](StringView name, bool currentValue, auto fnSetValue)
			{
				nodeSettingNode->addChild(CreateBoolPropertyNode(name, currentValue, fnSetValue));
			};
		fnAddBoolChild(U"isHitTarget", node->isHitTarget().getBool(), [node](bool value) { node->setIsHitTarget(value); });
		fnAddBoolChild(U"inheritsChildrenHoveredState", node->inheritsChildrenHoveredState(), [node](bool value) { node->setInheritsChildrenHoveredState(value); });
		fnAddBoolChild(U"inheritsChildrenPressedState", node->inheritsChildrenPressedState(), [node](bool value) { node->setInheritsChildrenPressedState(value); });
		fnAddBoolChild(U"interactable", node->interactable().getBool(), [node](bool value) { node->setInteractable(value); });
		fnAddBoolChild(U"horizontalScrollable", node->horizontalScrollable(), [node](bool value) { node->setHorizontalScrollable(value); });
		fnAddBoolChild(U"verticalScrollable", node->verticalScrollable(), [node](bool value) { node->setVerticalScrollable(value); });
		fnAddBoolChild(U"clippingEnabled", node->clippingEnabled().getBool(), [node](bool value) { node->setClippingEnabled(value); });

		nodeSettingNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);

		return nodeSettingNode;
	}

	enum class LayoutType
	{
		FlowLayout,
		HorizontalLayout,
		VerticalLayout,
	};

	[[nodiscard]]
	std::shared_ptr<Node> createLayoutNode(const std::shared_ptr<Node>& node)
	{
		auto layoutNode = Node::Create(
			U"Layout",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.margin = LRTB{ 0, 0, 0, 8 },
			});
		layoutNode->setLayout(VerticalLayout{ .padding = LRTB{ 0, 0, 0, 8 } });
		layoutNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);
		layoutNode->addChild(CreateHeadingNode(U"Layout (children)", ColorF{ 0.5, 0.3, 0.3 }));
		const auto fnAddChild =
			[&layoutNode](StringView name, const auto& value, auto fnSetValue)
			{
				layoutNode->addChild(CreatePropertyNode(name, Format(value), fnSetValue));
			};
		const auto fnAddVec2Child =
			[&layoutNode](StringView name, const Vec2& currentValue, auto fnSetValue)
			{
				layoutNode->addChild(CreateVec2PropertyNode(name, currentValue, fnSetValue));
			};
		const auto fnAddLRTBChild =
			[&layoutNode](StringView name, const LRTB& currentValue, auto fnSetValue)
			{
				layoutNode->addChild(CreateLRTBPropertyNode(name, currentValue, fnSetValue));
			};
		const auto fnAddEnumChild =
			[this, &layoutNode]<typename EnumType>(const String & name, EnumType currentValue, auto fnSetValue)
		{
			auto fnSetEnumValue = [fnSetValue = std::move(fnSetValue), currentValue](StringView value) { fnSetValue(StringToEnum<EnumType>(value, currentValue)); };
			layoutNode->addChild(CreateEnumPropertyNode(name, EnumToString(currentValue), fnSetEnumValue, m_contextMenu, EnumNames<EnumType>()));
		};
		if (const auto pFlowLayout = node->flowLayout())
		{
			fnAddEnumChild(
				U"type",
				LayoutType::FlowLayout,
				[this, node](LayoutType type)
				{
					switch (type)
					{
					case LayoutType::FlowLayout:
						break;
					case LayoutType::HorizontalLayout:
						node->setLayout(HorizontalLayout{});
						refreshInspector(); // 項目に変更があるため更新
						break;
					case LayoutType::VerticalLayout:
						node->setLayout(VerticalLayout{});
						refreshInspector(); // 項目に変更があるため更新
						break;
					}
				});
			fnAddLRTBChild(U"padding", pFlowLayout->padding, [this, node](const LRTB& value) { auto newLayout = *node->flowLayout(); newLayout.padding = value; node->setLayout(newLayout); });
		}
		else if (const auto pHorizontalLayout = node->horizontalLayout())
		{
			fnAddEnumChild(
				U"type",
				LayoutType::HorizontalLayout,
				[this, node](LayoutType type)
				{
					switch (type)
					{
					case LayoutType::FlowLayout:
						node->setLayout(FlowLayout{});
						refreshInspector(); // 項目に変更があるため更新
						break;
					case LayoutType::HorizontalLayout:
						break;
					case LayoutType::VerticalLayout:
						node->setLayout(VerticalLayout{});
						refreshInspector(); // 項目に変更があるため更新
						break;
					}
				});
			fnAddVec2Child(U"padding (left, right)", Vec2{ pHorizontalLayout->padding.left, pHorizontalLayout->padding.right }, [this, node](const Vec2& value) { auto newLayout = *node->horizontalLayout(); newLayout.padding.left = value.x; newLayout.padding.right = value.y; node->setLayout(newLayout); });
			fnAddVec2Child(U"padding (top, bottom)", Vec2{ pHorizontalLayout->padding.top, pHorizontalLayout->padding.bottom }, [this, node](const Vec2& value) { auto newLayout = *node->horizontalLayout(); newLayout.padding.top = value.x; newLayout.padding.bottom = value.y; node->setLayout(newLayout); });
			fnAddEnumChild(U"verticalAlign", pHorizontalLayout->verticalAlign, [this, node](VerticalAlign value) { auto newLayout = *node->horizontalLayout(); newLayout.verticalAlign = value; node->setLayout(newLayout); });
		}
		else if (const auto pVerticalLayout = node->verticalLayout())
		{
			fnAddEnumChild(
				U"type",
				LayoutType::VerticalLayout,
				[this, node](LayoutType type)
				{
					switch (type)
					{
					case LayoutType::FlowLayout:
						node->setLayout(FlowLayout{});
						refreshInspector(); // 項目に変更があるため更新
						break;
					case LayoutType::HorizontalLayout:
						node->setLayout(HorizontalLayout{});
						refreshInspector(); // 項目に変更があるため更新
						break;
					case LayoutType::VerticalLayout:
						break;
					}
				});
			fnAddVec2Child(U"padding (left, right)", Vec2{ pVerticalLayout->padding.left, pVerticalLayout->padding.right }, [this, node](const Vec2& value) { auto newLayout = *node->verticalLayout(); newLayout.padding.left = value.x; newLayout.padding.right = value.y; node->setLayout(newLayout); });
			fnAddVec2Child(U"padding (top, bottom)", Vec2{ pVerticalLayout->padding.top, pVerticalLayout->padding.bottom }, [this, node](const Vec2& value) { auto newLayout = *node->verticalLayout(); newLayout.padding.top = value.x; newLayout.padding.bottom = value.y; node->setLayout(newLayout); });
			fnAddEnumChild(U"horizontalAlign", pVerticalLayout->horizontalAlign, [this, node](HorizontalAlign value) { auto newLayout = *node->verticalLayout(); newLayout.horizontalAlign = value; node->setLayout(newLayout); });
		}
		else
		{
			throw Error{ U"Unknown layout type" };
		}

		layoutNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);

		return layoutNode;
	}

	enum class ConstraintType
	{
		BoxConstraint,
		AnchorConstraint,
	};

	[[nodiscard]]
	std::shared_ptr<Node> createConstraintNode(const std::shared_ptr<Node>& node)
	{
		auto constraintNode = Node::Create(
			U"Constraint",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.margin = LRTB{ 0, 0, 0, 8 },
			});
		constraintNode->setLayout(VerticalLayout{ .padding = LRTB{ 0, 0, 0, 8 } });
		constraintNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

		constraintNode->addChild(CreateHeadingNode(U"Constraint", ColorF{ 0.5, 0.3, 0.3 }));

		const auto fnAddChild =
			[&constraintNode](StringView name, const auto& value, auto fnSetValue)
			{
				constraintNode->addChild(CreatePropertyNode(name, Format(value), fnSetValue));
			};
		const auto fnAddEnumChild =
			[this, &constraintNode]<typename EnumType>(const String & name, EnumType currentValue, auto fnSetValue)
			{
				auto fnSetEnumValue = [fnSetValue = std::move(fnSetValue), currentValue](StringView value) { fnSetValue(StringToEnum<EnumType>(value, currentValue)); };
				constraintNode->addChild(CreateEnumPropertyNode(name, EnumToString(currentValue), fnSetEnumValue, m_contextMenu, EnumNames<EnumType>()));
			};
		const auto fnAddVec2Child =
			[&constraintNode](StringView name, const Vec2& currentValue, auto fnSetValue)
			{
				constraintNode->addChild(CreateVec2PropertyNode(name, currentValue, fnSetValue));
			};

		if (const auto pBoxConstraint = node->boxConstraint())
		{
			fnAddEnumChild(
				U"type",
				ConstraintType::BoxConstraint,
				[this, node](ConstraintType type)
				{
					switch (type)
					{
					case ConstraintType::BoxConstraint:
						break;
					case ConstraintType::AnchorConstraint:
						node->setConstraint(AnchorConstraint
						{
							.anchorMin = Anchor::MiddleCenter,
							.anchorMax = Anchor::MiddleCenter,
							.posDelta = Vec2::Zero(),
							.sizeDelta = node->layoutAppliedRect().size,
							.pivot = Vec2{ 0.5, 0.5 },
						});
						refreshInspector(); // 項目に変更があるため更新
						break;
					}
				});
			fnAddVec2Child(U"sizeRatio", pBoxConstraint->sizeRatio, [this, node](const Vec2& value) { auto newConstraint = *node->boxConstraint(); newConstraint.sizeRatio = value; node->setConstraint(newConstraint); });
			fnAddVec2Child(U"sizeDelta", pBoxConstraint->sizeDelta, [this, node](const Vec2& value) { auto newConstraint = *node->boxConstraint(); newConstraint.sizeDelta = value; node->setConstraint(newConstraint); });
			fnAddVec2Child(U"margin (left, right)", Vec2{ pBoxConstraint->margin.left, pBoxConstraint->margin.right }, [this, node](const Vec2& value) { auto newConstraint = *node->boxConstraint(); newConstraint.margin.left = value.x; newConstraint.margin.right = value.y; node->setConstraint(newConstraint); });
			fnAddVec2Child(U"margin (top, bottom)", Vec2{ pBoxConstraint->margin.top, pBoxConstraint->margin.bottom }, [this, node](const Vec2& value) { auto newConstraint = *node->boxConstraint(); newConstraint.margin.top = value.x; newConstraint.margin.bottom = value.y; node->setConstraint(newConstraint); });
		}
		else if (const auto pAnchorConstraint = node->anchorConstraint())
		{
			auto setDouble = [this, node](auto setter)
			{
				return [this, node, setter](StringView s)
				{
					if (auto optVal = ParseOpt<double>(s))
					{
						if (auto ac = node->anchorConstraint())
						{
							auto copy = *ac;
							setter(copy, *optVal);
							node->setConstraint(copy);
							m_canvas->refreshLayout();
						}
					}
				};
			};
			auto setVec2 = [this, node](auto setter)
			{
				return [this, node, setter](const Vec2& val)
				{
					if (auto ac = node->anchorConstraint())
					{
						auto copy = *ac;
						setter(copy, val);
						node->setConstraint(copy);
						m_canvas->refreshLayout();
					}
				};
			};

			fnAddEnumChild(
				U"type",
				ConstraintType::AnchorConstraint,
				[this, node](ConstraintType type)
				{
					switch (type)
					{
					case ConstraintType::BoxConstraint:
						node->setConstraint(BoxConstraint
						{
							.sizeRatio = Vec2::Zero(),
							.sizeDelta = node->rect().size,
						});
						refreshInspector(); // 項目に変更があるため更新
						break;
					case ConstraintType::AnchorConstraint:
						break;
					}
				}
			);

			const AnchorPreset anchorPreset =
				pAnchorConstraint->isCustomAnchorInEditor
					? AnchorPreset::Custom
					: ToAnchorPreset(pAnchorConstraint->anchorMin, pAnchorConstraint->anchorMax, pAnchorConstraint->pivot);

			fnAddEnumChild(
				U"anchor",
				anchorPreset,
				[this, node](AnchorPreset preset)
				{
					if (const auto pAnchorConstraint = node->anchorConstraint())
					{
						auto copy = *pAnchorConstraint;
						if (const auto tuple = FromAnchorPreset(preset))
						{
							// プリセットを選んだ場合
							std::tie(copy.anchorMin, copy.anchorMax, copy.pivot) = *tuple;
							copy.isCustomAnchorInEditor = false;
						}
						else
						{
							// Customを選んだ場合
							copy.isCustomAnchorInEditor = true;
						}

						// 変更がある場合のみ更新
						if (copy != *pAnchorConstraint)
						{
							if (!copy.isCustomAnchorInEditor)
							{
								const auto beforePreset = ToAnchorPreset(pAnchorConstraint->anchorMin, pAnchorConstraint->anchorMax, pAnchorConstraint->pivot);

								// 横ストレッチに変更した場合はleftとrightを0にする
								const auto fnIsHorizontalStretch = [](AnchorPreset preset)
									{
										return preset == AnchorPreset::StretchTop ||
											preset == AnchorPreset::StretchMiddle ||
											preset == AnchorPreset::StretchBottom ||
											preset == AnchorPreset::StretchFull;
									};
								if (!fnIsHorizontalStretch(beforePreset) && fnIsHorizontalStretch(preset))
								{
									copy.posDelta.x = 0;
									copy.sizeDelta.x = 0;
								}

								// 縦ストレッチに変更した場合はtopとbottomを0にする
								const auto fnIsVerticalStretch = [](AnchorPreset preset)
									{
										return preset == AnchorPreset::StretchLeft ||
											preset == AnchorPreset::StretchCenter ||
											preset == AnchorPreset::StretchRight ||
											preset == AnchorPreset::StretchFull;
									};
								if (!fnIsVerticalStretch(beforePreset) && fnIsVerticalStretch(preset))
								{
									copy.posDelta.y = 0;
									copy.sizeDelta.y = 0;
								}
							}

							node->setConstraint(copy);
							m_canvas->refreshLayout();
							refreshInspector(); // 項目に変更があるため更新
						}
					}
				}
			);
			switch (anchorPreset)
			{
			case AnchorPreset::TopLeft:
				fnAddChild(U"top", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
				fnAddChild(U"left", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
				fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
				break;

			case AnchorPreset::TopCenter:
				fnAddChild(U"top", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
				fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
				fnAddChild(U"xDelta", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
				break;

			case AnchorPreset::TopRight:
				fnAddChild(U"top", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
				fnAddChild(U"right", -pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = -v; }));
				fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
				break;

			case AnchorPreset::MiddleLeft:
				fnAddChild(U"left", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
				fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
				fnAddChild(U"yDelta", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
				break;

			case AnchorPreset::MiddleCenter:
				fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
				fnAddVec2Child(U"posDelta", pAnchorConstraint->posDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.posDelta = v; }));
				break;

			case AnchorPreset::MiddleRight:
				fnAddChild(U"right", -pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = -v; }));
				fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
				fnAddChild(U"yDelta", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
				break;

			case AnchorPreset::BottomLeft:
				fnAddChild(U"left", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
				fnAddChild(U"bottom", -pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = -v; }));
				fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
				break;

			case AnchorPreset::BottomCenter:
				fnAddChild(U"bottom", -pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = -v; }));
				fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
				fnAddChild(U"xDelta", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
				break;

			case AnchorPreset::BottomRight:
				fnAddChild(U"right", -pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = -v; }));
				fnAddChild(U"bottom", -pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = -v; }));
				fnAddVec2Child(U"size", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& v) { c.sizeDelta = v; }));
				break;

			case AnchorPreset::StretchTop:
				fnAddChild(U"top", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
				fnAddChild(U"left", pAnchorConstraint->posDelta.x,
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldLeft = pAnchorConstraint->posDelta.x;
							double delta = oldLeft - v;
							c.posDelta.x = v;
							c.sizeDelta.x += delta;
						}));
				fnAddChild(U"right", -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x),
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldRight = -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x);
							double delta = v - oldRight;
							c.sizeDelta.x -= delta;
						}));
				fnAddChild(U"height", pAnchorConstraint->sizeDelta.y, setDouble([](AnchorConstraint& c, double v) { c.sizeDelta.y = v; }));
				break;

			case AnchorPreset::StretchMiddle:
				fnAddChild(U"left", pAnchorConstraint->posDelta.x,
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldLeft = pAnchorConstraint->posDelta.x;
							double delta = oldLeft - v;
							c.posDelta.x = v;
							c.sizeDelta.x += delta;
						}));
				fnAddChild(U"right", -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x),
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldRight = -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x);
							double delta = v - oldRight;
							c.sizeDelta.x -= delta;
						}));
				fnAddChild(U"height", pAnchorConstraint->sizeDelta.y, setDouble([](AnchorConstraint& c, double v) { c.sizeDelta.y = v; }));
				fnAddChild(U"yDelta", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = v; }));
				break;

			case AnchorPreset::StretchBottom:
				fnAddChild(U"left", pAnchorConstraint->posDelta.x,
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldLeft = pAnchorConstraint->posDelta.x;
							double delta = oldLeft - v;
							c.posDelta.x = v;
							c.sizeDelta.x += delta;
						}));
				fnAddChild(U"right", -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x),
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldRight = -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x);
							double delta = v - oldRight;
							c.sizeDelta.x -= delta;
						}));
				fnAddChild(U"bottom", pAnchorConstraint->posDelta.y, setDouble([](AnchorConstraint& c, double v) { c.posDelta.y = -v; }));
				fnAddChild(U"height", pAnchorConstraint->sizeDelta.y, setDouble([](AnchorConstraint& c, double v) { c.sizeDelta.y = v; }));
				break;

			case AnchorPreset::StretchLeft:
				fnAddChild(U"top", pAnchorConstraint->posDelta.y,
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldTop = pAnchorConstraint->posDelta.y;
							double delta = oldTop - v;
							c.posDelta.y = v;
							c.sizeDelta.y += delta;
						}));
				fnAddChild(U"bottom", -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y),
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldBottom = -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y);
							double delta = v - oldBottom;
							c.sizeDelta.y -= delta;
						}));
				fnAddChild(U"left", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
				fnAddChild(U"width", pAnchorConstraint->sizeDelta.x, setDouble([](AnchorConstraint& c, double v) { c.sizeDelta.x = v; }));
				break;

			case AnchorPreset::StretchCenter:
				fnAddChild(U"top", pAnchorConstraint->posDelta.y,
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldTop = pAnchorConstraint->posDelta.y;
							double delta = oldTop - v;
							c.posDelta.y = v;
							c.sizeDelta.y += delta;
						}));
				fnAddChild(U"bottom", -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y),
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldBottom = -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y);
							double delta = v - oldBottom;
							c.sizeDelta.y -= delta;
						}));
				fnAddChild(U"width", pAnchorConstraint->sizeDelta.x, setDouble([](AnchorConstraint& c, double v) { c.sizeDelta.x = v; }));
				fnAddChild(U"xDelta", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = v; }));
				break;

			case AnchorPreset::StretchRight:
				fnAddChild(U"top", pAnchorConstraint->posDelta.y,
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldTop = pAnchorConstraint->posDelta.y;
							double delta = oldTop - v;
							c.posDelta.y = v;
							c.sizeDelta.y += delta;
						}));
				fnAddChild(U"bottom", -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y),
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldBottom = -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y);
							double delta = v - oldBottom;
							c.sizeDelta.y -= delta;
						}));
				fnAddChild(U"right", pAnchorConstraint->posDelta.x, setDouble([](AnchorConstraint& c, double v) { c.posDelta.x = -v; }));
				fnAddChild(U"width", pAnchorConstraint->sizeDelta.x, setDouble([](AnchorConstraint& c, double v) { c.sizeDelta.x = v; }));
				break;

			case AnchorPreset::StretchFull:
				fnAddChild(U"left", pAnchorConstraint->posDelta.x,
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldLeft = pAnchorConstraint->posDelta.x;
							double delta = oldLeft - v;
							c.posDelta.x = v;
							c.sizeDelta.x += delta;
						}));
				fnAddChild(U"right", -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x),
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldRight = -(pAnchorConstraint->posDelta.x + pAnchorConstraint->sizeDelta.x);
							double delta = v - oldRight;
							c.sizeDelta.x -= delta;
						}));
				fnAddChild(U"top", pAnchorConstraint->posDelta.y,
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldTop = pAnchorConstraint->posDelta.y;
							double delta = oldTop - v;
							c.posDelta.y = v;
							c.sizeDelta.y += delta;
						}));
				fnAddChild(U"bottom", -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y),
					setDouble([pAnchorConstraint](AnchorConstraint& c, double v)
						{
							double oldBottom = -(pAnchorConstraint->posDelta.y + pAnchorConstraint->sizeDelta.y);
							double delta = v - oldBottom;
							c.sizeDelta.y -= delta;
						}));
				break;

			default:
				fnAddVec2Child(U"anchorMin", pAnchorConstraint->anchorMin, setVec2([](AnchorConstraint& c, const Vec2& val) { c.anchorMin = val; }));
				fnAddVec2Child(U"anchorMax", pAnchorConstraint->anchorMax, setVec2([](AnchorConstraint& c, const Vec2& val) { c.anchorMax = val; }));
				fnAddVec2Child(U"pivot", pAnchorConstraint->pivot, setVec2([](AnchorConstraint& c, const Vec2& val) { c.pivot = val; }));
				fnAddVec2Child(U"posDelta", pAnchorConstraint->posDelta, setVec2([](AnchorConstraint& c, const Vec2& val) { c.posDelta = val; }));
				fnAddVec2Child(U"sizeDelta", pAnchorConstraint->sizeDelta, setVec2([](AnchorConstraint& c, const Vec2& val) { c.sizeDelta = val; }));
				break;
			}
		}
		else
		{
			throw Error{ U"Unknown constraint type" };
		}

		constraintNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);

		return constraintNode;
	}

	[[nodiscard]]
	std::shared_ptr<Node> createTransformEffectNode(TransformEffect* const pTransformEffect)
	{
		auto transformEffectNode = Node::Create(
			U"TransformEffect",
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.margin = LRTB{ 0, 0, 0, 8 },
			});
		transformEffectNode->setLayout(VerticalLayout{ .padding = LRTB{ 0, 0, 0, 8 } });
		transformEffectNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

		transformEffectNode->addChild(CreateHeadingNode(U"TransformEffect", ColorF{ 0.3, 0.5, 0.3 }));

		const auto fnAddChild =
			[&transformEffectNode](StringView name, const SmoothProperty<double>& currentValue, auto fnSetValue)
			{
				transformEffectNode->addChild(CreatePropertyNode(name, currentValue.propertyValueString(), [fnSetValue](StringView value) { fnSetValue(ParseOr<double>(value, 0.0)); }));
			};
		const auto fnAddVec2Child =
			[&transformEffectNode](StringView name, const SmoothProperty<Vec2>& currentValue, auto fnSetValue)
			{
				// TODO: hovered/pressed/selectedの値を入出力可能にする
				transformEffectNode->addChild(CreateVec2PropertyNode(name, currentValue.propertyValue().defaultValue, fnSetValue));
			};
		fnAddVec2Child(U"position", pTransformEffect->position(), [this, pTransformEffect](const Vec2& value) { pTransformEffect->setPosition(value); m_canvas->refreshLayout(); });
		fnAddVec2Child(U"scale", pTransformEffect->scale(), [this, pTransformEffect](const Vec2& value) { pTransformEffect->setScale(value); m_canvas->refreshLayout(); });
		fnAddVec2Child(U"pivot", pTransformEffect->pivot(), [this, pTransformEffect](const Vec2& value) { pTransformEffect->setPivot(value); m_canvas->refreshLayout(); });
		fnAddChild(U"rotation", pTransformEffect->rotation(), [this, pTransformEffect](double value) { pTransformEffect->setRotation(value); m_canvas->refreshLayout(); });

		transformEffectNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);

		return transformEffectNode;
	}

	[[nodiscard]]
	std::shared_ptr<Node> createComponentNode(const std::shared_ptr<Node>& node, const std::shared_ptr<ComponentBase>& component)
	{
		auto componentNode = Node::Create(
			component->type(),
			BoxConstraint
			{
				.sizeRatio = Vec2{ 1, 0 },
				.margin = LRTB{ 0, 0, 0, 8 },
			});
		componentNode->setLayout(VerticalLayout{ .padding = LRTB{ 0, 0, 0, 8 } });
		componentNode->emplaceComponent<RectRenderer>(ColorF{ 0.3, 0.3 }, ColorF{ 1.0, 0.3 }, 1.0, 3.0);

		const auto headingNode = componentNode->addChild(CreateHeadingNode(component->type(), ColorF{ 0.3, 0.3, 0.5 }));
		headingNode->emplaceComponent<ContextMenuOpener>(
			m_contextMenu,
			Array<MenuElement>
			{
				MenuItem{ U"{} を削除"_fmt(component->type()), U"", [this, node, component] { node->removeComponent(component); refreshInspector(); } },
				MenuItem{ U"{} を上へ移動"_fmt(component->type()), U"", [this, node, component] { node->moveComponentUp(component); refreshInspector(); } },
				MenuItem{ U"{} を下へ移動"_fmt(component->type()), U"", [this, node, component] { node->moveComponentDown(component); refreshInspector(); } },
			});

		for (const auto& property : component->properties())
		{
			switch (property->editType())
			{
			case PropertyEditType::Text:
				componentNode->addChild(
					CreatePropertyNode(
						property->name(),
						property->propertyValueString(),
						[property](StringView value) { property->trySetPropertyValueString(value); }));
				break;
			case PropertyEditType::Bool:
				componentNode->addChild(
					CreateBoolPropertyNode(
						property->name(),
						ParseOr<bool>(property->propertyValueString(), false),
						[property](bool value) { property->trySetPropertyValueString(Format(value)); }));
				break;
			case PropertyEditType::Vec2:
				componentNode->addChild(
					CreateVec2PropertyNode(
						property->name(),
						ParseOr<Vec2>(property->propertyValueString(), Vec2{ 0, 0 }),
						[property](const Vec2& value) { property->trySetPropertyValueString(Format(value)); }));
				break;
			case PropertyEditType::Color:
				componentNode->addChild(
					CreateColorPropertyNode(
						property->name(),
						ParseOr<ColorF>(property->propertyValueString(), ColorF{ 0, 0, 0, 1 }),
						[property](const ColorF& value) { property->trySetPropertyValueString(Format(value)); }));
				break;
			case PropertyEditType::LRTB:
				componentNode->addChild(
					CreateLRTBPropertyNode(
						property->name(),
						ParseOr<LRTB>(property->propertyValueString(), LRTB{ 0, 0, 0, 0 }),
						[property](const LRTB& value) { property->trySetPropertyValueString(Format(value)); }));
				break;
			case PropertyEditType::Enum:
				componentNode->addChild(
					CreateEnumPropertyNode(
						property->name(),
						property->propertyValueString(),
						[property](StringView value) { property->trySetPropertyValueString(value); },
						m_contextMenu,
						property->enumCandidates()));
				break;
			}
		}

		componentNode->setBoxConstraintToFitToChildren(FitTarget::HeightOnly);

		return componentNode;
	}

	void clearTargetNode()
	{
		setTargetNode(nullptr);
	}

	void update()
	{
	}

	[[nodiscard]]
	const std::shared_ptr<Node>& inspectorFrameNode() const
	{
		return m_inspectorFrameNode;
	}
};

void Label::draw(const Node& node) const
{
	const auto& text = m_text.value();

	if (text.empty())
	{
		return;
	}

	const Vec2& effectScale = node.effectScale();
	const Vec2& spacing = m_spacing.value();
	const LRTB& padding = m_padding.value();
	const double leftPadding = padding.left * effectScale.x;
	const double rightPadding = padding.right * effectScale.x;
	const double topPadding = padding.top * effectScale.y;
	const double bottomPadding = padding.bottom * effectScale.y;

	// stretchedはtop,right,bottom,leftの順
	const RectF rect = node.rect().stretched(-topPadding, -rightPadding, -bottomPadding, -leftPadding);

	m_cache.refreshIfDirty(
		text,
		m_fontAssetName.value(),
		m_fontSize.value(),
		spacing,
		m_horizontalOverflow.value(),
		m_verticalOverflow.value(),
		rect.size / effectScale);

	double posY;
	const VerticalAlign& verticalAlign = m_verticalAlign.value();
	switch (verticalAlign)
	{
	case VerticalAlign::Top:
		posY = rect.y;
		break;
	case VerticalAlign::Middle:
		posY = rect.y + (rect.h - m_cache.regionSize.y * effectScale.y) / 2;
		break;
	case VerticalAlign::Bottom:
		posY = rect.y + rect.h - m_cache.regionSize.y * effectScale.y;
		break;
	default:
		throw Error{ U"Invalid VerticalAlign: {}"_fmt(static_cast<std::underlying_type_t<VerticalAlign>>(verticalAlign)) };
	}

	const ScopedCustomShader2D shader{ Font::GetPixelShader(m_cache.fontMethod) };
	const HorizontalAlign& horizontalAlign = m_horizontalAlign.value();
	for (const auto& lineCache : m_cache.lineCaches)
	{
		double posX;
		switch (horizontalAlign)
		{
		case HorizontalAlign::Left:
			posX = rect.x;
			break;
		case HorizontalAlign::Center:
			posX = rect.x + (rect.w - lineCache.width * effectScale.x) / 2;
			break;
		case HorizontalAlign::Right:
			posX = rect.x + rect.w - lineCache.width * effectScale.x;
			break;
		default:
			throw Error{ U"Invalid HorizontalAlign: {}"_fmt(static_cast<std::underlying_type_t<HorizontalAlign>>(horizontalAlign)) };
		}

		for (const auto& glyph : lineCache.glyphs)
		{
			if (glyph.codePoint == U'\n')
			{
				continue;
			}
			const Vec2 pos{ posX, posY + lineCache.offsetY * effectScale.y };
			const ColorF& color = m_color.value();
			glyph.texture.scaled(m_cache.scale * effectScale).draw(pos + glyph.getOffset(m_cache.scale) * effectScale, color);
			posX += (glyph.xAdvance * m_cache.scale + spacing.x) * effectScale.x;
		}
	}
}

class Editor
{
private:
	std::shared_ptr<Canvas> m_canvas;
	std::shared_ptr<Canvas> m_editorCanvas;
	std::shared_ptr<Canvas> m_editorOverlayCanvas;
	std::shared_ptr<ContextMenu> m_contextMenu;
	Hierarchy m_hierarchy;
	Inspector m_inspector;
	MenuBar m_menuBar;
	Size m_prevSceneSize;
	std::weak_ptr<Node> m_prevSelectedNode;
	bool m_prevSelectedNodeExists = false;
	Optional<String> m_filePath = none;
	Vec2 m_scrollOffset = Vec2::Zero();
	double m_scrollScale = 1.0;

public:
	Editor()
		: m_canvas(Canvas::Create())
		, m_editorCanvas(Canvas::Create())
		, m_editorOverlayCanvas(Canvas::Create())
		, m_contextMenu(std::make_shared<ContextMenu>(m_editorOverlayCanvas, U"EditorContextMenu"))
		, m_hierarchy(m_canvas, m_editorCanvas, m_contextMenu)
		, m_inspector(m_canvas, m_editorCanvas, m_contextMenu, [this] { m_hierarchy.refreshNodeNames(); })
		, m_menuBar(m_editorCanvas, m_contextMenu)
		, m_prevSceneSize(Scene::Size())
	{
		m_menuBar.addMenuCategory(
			U"File",
			U"ファイル",
			Array<MenuElement>
			{
				MenuItem{ U"新規作成", U"Ctrl+N", [this] { onClickMenuFileNew(); } },
				MenuSeparator{},
				MenuItem{ U"開く", U"Ctrl+O", [this] { onClickMenuFileOpen(); } },
				MenuItem{ U"保存", U"Ctrl+S", [this] { onClickMenuFileSave(); } },
				MenuItem{ U"名前を付けて保存", U"Ctrl+Shift+S", [this] { onClickMenuFileSaveAs(); } },
				MenuSeparator{},
				MenuItem{ U"終了", U"Alt+F4", [this] { onClickMenuFileExit(); } },
			});
		m_menuBar.addMenuCategory(
			U"Edit",
			U"編集",
			{
				MenuItem{ U"切り取り", U"Ctrl+X", [this] { onClickMenuEditCut(); }, [this] { return m_hierarchy.hasSelection(); } },
				MenuItem{ U"コピー", U"Ctrl+C", [this] { onClickMenuEditCopy(); }, [this] { return m_hierarchy.hasSelection(); } },
				MenuItem{ U"貼り付け", U"Ctrl+V", [this] { onClickMenuEditPaste(); }, [this] { return m_hierarchy.canPaste(); } },
				MenuItem{ U"複製を作成", U"Ctrl+D", [this] { onClickMenuEditDuplicate(); }, [this] { return m_hierarchy.hasSelection(); } },
				MenuItem{ U"削除", U"Delete", [this] { onClickMenuEditDelete(); }, [this] { return m_hierarchy.hasSelection(); } },
				MenuSeparator{},
				MenuItem{ U"すべて選択", U"Ctrl+A", [this] { m_hierarchy.selectAll(); } },
			});
		m_menuBar.addMenuCategory(
			U"View",
			U"表示",
			{
			});
	}

	void update()
	{
		CanvasUpdateContext context{};
		m_editorOverlayCanvas->update(&context);
		m_editorCanvas->update(&context);
		m_canvas->update(&context);

		if (!context.isScrollableHovered())
		{
			// マウス座標を中心に拡大縮小
			const Vec2 beforeOffset = m_scrollOffset;
			const double beforeScale = m_scrollScale;
			const double scaleFactor = std::exp(-0.2 * Mouse::Wheel());
			m_scrollScale = Clamp(beforeScale * scaleFactor, 0.1, 10.0);
			const Vec2 cursorPos = Cursor::PosF();
			const Vec2 cursorPosInWorldBefore = (cursorPos + m_scrollOffset) / beforeScale;
			const Vec2 cursorPosInWorldAfter = (cursorPos + m_scrollOffset) / m_scrollScale;
			m_scrollOffset += (cursorPosInWorldBefore - cursorPosInWorldAfter) * m_scrollScale;
			if (beforeOffset != m_scrollOffset || beforeScale != m_scrollScale)
			{
				m_canvas->setOffsetScale(-m_scrollOffset, Vec2::All(m_scrollScale));
			}
		}

		m_contextMenu->update();
		m_menuBar.update();
		m_hierarchy.update();
		m_inspector.update();

		const auto selectedNode = m_hierarchy.selectedNode().lock();
		if (selectedNode != m_prevSelectedNode.lock() ||
			(!selectedNode && m_prevSelectedNodeExists))
		{
			m_inspector.setTargetNode(selectedNode);
		}

		const auto sceneSize = Scene::Size();
		if (m_prevSceneSize != sceneSize)
		{
			refreshLayout();
			m_prevSceneSize = sceneSize;
		}

		// ショートカットキー
		const bool isWindowActive = Window::GetState().focused;
		if (isWindowActive)
		{
			const bool ctrl = KeyControl.pressed();
			const bool alt = KeyAlt.pressed();
			const bool shift = KeyShift.pressed();

			// Ctrl + ○○
			if (ctrl && !alt && !shift)
			{
				if (KeyN.down())
				{
					onClickMenuFileNew();
				}
				else if (KeyO.down())
				{
					onClickMenuFileOpen();
				}
				else if (KeyS.down())
				{
					onClickMenuFileSave();
				}
				else if (KeyA.down())
				{
					m_hierarchy.selectAll();
				}
			}

			// Ctrl + Shift + ○○
			if (ctrl && !alt && shift)
			{
				if (KeyS.down())
				{
					onClickMenuFileSaveAs();
				}
			}

			// Hierarchy操作
			// (テキストボックス編集中は実行しない)
			if (context.editingTextBox.expired())
			{
				// Ctrl + ○○
				if (ctrl && !alt && !shift)
				{
					if (KeyC.down())
					{
						m_hierarchy.onClickCopy();
					}
					else if (KeyV.down())
					{
						m_hierarchy.onClickPaste();
					}
					else if (KeyX.down())
					{
						m_hierarchy.onClickCut();
					}
					else if (KeyD.down())
					{
						m_hierarchy.onClickDuplicate();
					}
				}

				// Alt + ○○
				if (!ctrl && alt && !shift)
				{
					if (KeyUp.down())
					{
						m_hierarchy.onClickMoveUp();
					}
					else if (KeyDown.down())
					{
						m_hierarchy.onClickMoveDown();
					}
				}

				// 単体キー
				if (!ctrl && !alt && !shift)
				{
					if (KeyDelete.down())
					{
						m_hierarchy.onClickDelete();
					}
				}
			}
		}

		m_prevSelectedNode = selectedNode;
		m_prevSelectedNodeExists = selectedNode != nullptr;
	}

	void draw() const
	{
		m_canvas->draw();
		constexpr double Thickness = 2.0;
		m_canvas->rootNode()->rect().stretched(Thickness / 2).drawFrame(Thickness, ColorF{ 1.0 });
		m_hierarchy.drawSelectedNodesGizmo();
		m_editorCanvas->draw();
		m_editorOverlayCanvas->draw();
	}

	const std::shared_ptr<Canvas>& canvas() const
	{
		return m_canvas;
	}

	const Hierarchy& hierarchy() const
	{
		return m_hierarchy;
	}

	const std::shared_ptr<Node>& rootNode() const
	{
		return m_canvas->rootNode();
	}

	void refreshLayout()
	{
		m_editorCanvas->refreshLayout();
		m_editorOverlayCanvas->refreshLayout();
		m_canvas->refreshLayout();
	}

	void refresh()
	{
		m_hierarchy.refreshNodeList();
		refreshLayout();
	}

	void onClickMenuFileNew()
	{
		m_filePath = none;
		m_canvas->removeChildrenAll();
		refresh();
	}

	void onClickMenuFileOpen()
	{
		if (const auto filePath = Dialog::OpenFile({ FileFilter::JSON() }))
		{
			JSON json;
			try
			{
				json = JSON::Load(*filePath);
			}
			catch (...)
			{
				System::MessageBoxOK(U"エラー", U"ファイルの読み込みに失敗しました", MessageBoxStyle::Error);
				return;
			}
			m_filePath = filePath;
			m_canvas->loadFromJSON(json);
			refresh();
		}
	}

	void onClickMenuFileSave()
	{
		Optional<String> filePath = m_filePath;
		if (filePath == none)
		{
			filePath = Dialog::SaveFile({ FileFilter::JSON() });
			if (filePath == none)
			{
				return;
			}
		}
		if (m_canvas->toJSON().save(*filePath))
		{
			m_filePath = filePath;
		}
		else
		{
			System::MessageBoxOK(U"エラー", U"保存に失敗しました", MessageBoxStyle::Error);
		}
	}

	void onClickMenuFileSaveAs()
	{
		if (const auto filePath = Dialog::SaveFile({ FileFilter::JSON() }))
		{
			if (m_canvas->toJSON().save(*filePath))
			{
				m_filePath = filePath;
			}
			else
			{
				System::MessageBoxOK(U"エラー", U"保存に失敗しました", MessageBoxStyle::Error);
			}
		}
	}

	void onClickMenuFileExit()
	{
		System::Exit();
	}

	void onClickMenuEditCut()
	{
		m_hierarchy.onClickCut();
	}

	void onClickMenuEditCopy()
	{
		m_hierarchy.onClickCopy();
	}

	void onClickMenuEditPaste()
	{
		m_hierarchy.onClickPaste();
	}

	void onClickMenuEditDuplicate()
	{
		m_hierarchy.onClickDuplicate();
	}

	void onClickMenuEditDelete()
	{
		m_hierarchy.onClickDelete();
	}

	void onClickMenuEditSelectAll()
	{
		m_hierarchy.selectAll();
	}
};

void Main()
{
	Window::SetStyle(WindowStyle::Sizable);
	Window::Resize(1280, 720);

	FontAsset::Register(U"Font", FontMethod::MSDF, 60);
	FontAsset::Register(U"Font14", FontMethod::MSDF, 14);
	FontAsset::Register(U"Font14Bold", FontMethod::MSDF, 14, Typeface::Bold);

	TextureAsset::Register(U"Chara", U"example/siv3d-kun.png");

	Editor editor;
	editor.rootNode()->setConstraint(AnchorConstraint
	{
		.anchorMin = Anchor::MiddleCenter,
		.anchorMax = Anchor::MiddleCenter,
		.posDelta = Vec2{ 0, 0 },
		.sizeDelta = Vec2{ 800, 600 },
	});
	editor.rootNode()->setLayout(FlowLayout{ .padding = { 10, 10, 10, 10 } });
	const auto textBoxNode = editor.rootNode()->emplaceChild(
		U"TextBox",
		AnchorConstraint
		{
			.anchorMin = Anchor::MiddleCenter,
			.anchorMax = Anchor::MiddleCenter,
			.posDelta = Vec2{ 0, 0 },
			.sizeDelta = Vec2{ 200, 46 },
			.pivot = Anchor::MiddleCenter,
		});
	const auto rectRenderer = textBoxNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ Palette::White }.withDisabled(ColorF{ 0.5 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ Palette::Skyblue, 0.0 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 2.0, 4.0);
	textBoxNode->emplaceComponent<TextBox>(U"Font14Bold", 24);
	textBoxNode->transformEffect().setScale(PropertyValue(Vec2::One()).withHover(Vec2{ 2.0, 2.0 }).withSelectedDefault(Vec2{ 2.0, 2.0 }).withSmoothTime(0.1));

	const auto charaNode = editor.rootNode()->emplaceChild(
		U"Sprite",
		AnchorConstraint
		{
			.anchorMin = Anchor::TopLeft,
			.anchorMax = Anchor::TopLeft,
			.posDelta = Vec2{ 0, 0 },
			.sizeDelta = Vec2{ 200, 200 },
			.pivot = Anchor::TopLeft,
		});
	charaNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ ColorF{ 0.0, 0.2 } }.withHover(ColorF{ Palette::Yellow, 0.2 }).withPressed(ColorF{ Palette::Red, 0.2 }).withSmoothTime(0.1));
	charaNode->emplaceComponent<Sprite>(U"Chara");

	const auto labelNode = editor.rootNode()->emplaceChild(
		U"Label",
		AnchorConstraint
		{
			.anchorMin = Anchor::TopLeft,
			.anchorMax = Anchor::TopLeft,
			.posDelta = Vec2{ 220, 0 },
			.sizeDelta = Vec2{ 200, 200 },
			.pivot = Anchor::TopLeft,
		});
	labelNode->emplaceComponent<Label>(U"← 左上にアンカーがある", U"Font", 24, Palette::White, HorizontalAlign::Left, VerticalAlign::Middle);

	/*for (int32 i = 0; i < 100; i++)
	{
		const auto buttonNode = editor.rootNode()->emplaceChild(
			U"Button{}"_fmt(i),
			BoxConstraint
			{
				.sizeRatio = Vec2{ 0, 0 },
				.sizeDelta = Vec2{ Random(70, 110), Random(20, 60) },
			},
			IsHitTargetYN::Yes,
			InheritChildrenStateFlags::None,
			RefreshesLayoutYN::No);
		buttonNode->emplaceComponent<RectRenderer>(PropertyValue<ColorF>{ Palette::Black }.withDisabled(ColorF{ 0.5 }).withSmoothTime(0.05), PropertyValue<ColorF>{ ColorF{ Palette::Skyblue, 0.0 } }.withHover(Palette::Skyblue).withSelectedDefault(Palette::Orange).withSmoothTime(0.05), 2.0, 4.0);
		buttonNode->emplaceComponent<Label>(U"Button{}"_fmt(i), U"Font14Bold", 24, Palette::White, HorizontalAlign::Center, VerticalAlign::Middle);
	}*/

	editor.refresh();

	Scene::SetBackground(ColorF{ 0.2, 0.2, 0.3 });

	while (System::Update())
	{
		editor.update();
		editor.draw();
	}
}
