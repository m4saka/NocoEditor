﻿#pragma once
#include <Siv3D.hpp>
#include "YN.hpp"
#include "InteractState.hpp"
#include "Serialization.hpp"

namespace noco
{
	template <class T>
	struct PropertyValue
	{
		T defaultValue;
		Optional<T> hoveredValue = none;
		Optional<T> pressedValue = none;
		Optional<T> disabledValue = none;
		Optional<T> selectedDefaultValue = none;
		Optional<T> selectedHoveredValue = none;
		Optional<T> selectedPressedValue = none;
		Optional<T> selectedDisabledValue = none;
		double smoothTime = 0.0;

		/*implicit*/ PropertyValue(const T& defaultValue)
			: defaultValue{ static_cast<T>(defaultValue) }
		{
		}

		template <class U>
		/*implicit*/ PropertyValue(const U& defaultValue) requires std::convertible_to<U, T>
			: defaultValue{ static_cast<T>(defaultValue) }
		{
		}

		/*implicit*/ PropertyValue(StringView defaultValue) requires std::same_as<T, String>
			: defaultValue{ static_cast<T>(String{ defaultValue }) }
		{
		}

		PropertyValue(const T& defaultValue, const Optional<T>& hoveredValue, const Optional<T>& pressedValue, const Optional<T>& disabledValue, double smoothTime = 0.0)
			: defaultValue{ defaultValue }
			, hoveredValue{ hoveredValue }
			, pressedValue{ pressedValue }
			, disabledValue{ disabledValue }
			, smoothTime{ smoothTime }
		{
		}

		[[nodiscard]]
		const T& value(InteractState interactState, SelectedYN selected) const
		{
			if (selected)
			{
				switch (interactState)
				{
				case InteractState::Default:
					if (selectedDefaultValue)
					{
						return *selectedDefaultValue;
					}
					break;
				case InteractState::Hovered:
					if (selectedHoveredValue)
					{
						return *selectedHoveredValue;
					}
					if (selectedDefaultValue)
					{
						return *selectedDefaultValue;
					}
					break;
				case InteractState::Pressed:
					if (selectedPressedValue)
					{
						return *selectedPressedValue;
					}
					if (selectedHoveredValue)
					{
						return *selectedHoveredValue;
					}
					if (selectedDefaultValue)
					{
						return *selectedDefaultValue;
					}
					break;
				case InteractState::Disabled:
					if (selectedDisabledValue)
					{
						return *selectedDisabledValue;
					}
					break;
				}
			}

			switch (interactState)
			{
			case InteractState::Default:
				break;
			case InteractState::Hovered:
				if (hoveredValue)
				{
					return *hoveredValue;
				}
				break;
			case InteractState::Pressed:
				if (pressedValue)
				{
					return *pressedValue;
				}
				if (hoveredValue)
				{
					return *hoveredValue;
				}
				break;
			case InteractState::Disabled:
				if (disabledValue)
				{
					return *disabledValue;
				}
				break;
			}
			return defaultValue;
		}

		[[nodiscard]]
		JSON toJSON() const
		{
			if constexpr (std::is_enum_v<T>)
			{
				if (!hoveredValue && !pressedValue && !disabledValue && smoothTime == 0.0)
				{
					return EnumToString(defaultValue);
				}
				JSON json;
				json[U"default"] = EnumToString(defaultValue);
				if (hoveredValue)
				{
					json[U"hovered"] = EnumToString(*hoveredValue);
				}
				if (pressedValue)
				{
					json[U"pressed"] = EnumToString(*pressedValue);
				}
				if (disabledValue)
				{
					json[U"disabled"] = EnumToString(*disabledValue);
				}
				if (selectedDefaultValue)
				{
					json[U"selectedDefault"] = EnumToString(*selectedDefaultValue);
				}
				if (selectedHoveredValue)
				{
					json[U"selectedHovered"] = EnumToString(*selectedHoveredValue);
				}
				if (selectedPressedValue)
				{
					json[U"selectedPressed"] = EnumToString(*selectedPressedValue);
				}
				if (selectedDisabledValue)
				{
					json[U"selectedDisabled"] = EnumToString(*selectedDisabledValue);
				}
				if (smoothTime != 0.0)
				{
					json[U"smoothTime"] = smoothTime;
				}
				return json;
			}
			else if constexpr (HasToJSON<T>)
			{
				if (!hoveredValue && !pressedValue && !disabledValue && smoothTime == 0.0)
				{
					return defaultValue.toJSON();
				}
				JSON json;
				json[U"default"] = defaultValue.toJSON();
				if (hoveredValue)
				{
					json[U"hovered"] = hoveredValue->toJSON();
				}
				if (pressedValue)
				{
					json[U"pressed"] = pressedValue->toJSON();
				}
				if (disabledValue)
				{
					json[U"disabled"] = disabledValue->toJSON();
				}
				if (selectedDefaultValue)
				{
					json[U"selectedDefault"] = selectedDefaultValue->toJSON();
				}
				if (selectedHoveredValue)
				{
					json[U"selectedHovered"] = selectedHoveredValue->toJSON();
				}
				if (selectedPressedValue)
				{
					json[U"selectedPressed"] = selectedPressedValue->toJSON();
				}
				if (selectedDisabledValue)
				{
					json[U"selectedDisabled"] = selectedDisabledValue->toJSON();
				}
				if (smoothTime != 0.0)
				{
					json[U"smoothTime"] = smoothTime;
				}
				return json;
			}
			else
			{
				if (!hoveredValue && !pressedValue && !disabledValue && smoothTime == 0.0)
				{
					return defaultValue;
				}
				JSON json;
				json[U"default"] = defaultValue;
				if (hoveredValue)
				{
					json[U"hovered"] = *hoveredValue;
				}
				if (pressedValue)
				{
					json[U"pressed"] = *pressedValue;
				}
				if (disabledValue)
				{
					json[U"disabled"] = *disabledValue;
				}
				if (selectedDefaultValue)
				{
					json[U"selectedDefault"] = *selectedDefaultValue;
				}
				if (selectedHoveredValue)
				{
					json[U"selectedHovered"] = *selectedHoveredValue;
				}
				if (selectedPressedValue)
				{
					json[U"selectedPressed"] = *selectedPressedValue;
				}
				if (selectedDisabledValue)
				{
					json[U"selectedDisabled"] = *selectedDisabledValue;
				}
				if (smoothTime != 0.0)
				{
					json[U"smoothTime"] = smoothTime;
				}
				return json;
			}
		}

		[[nodiscard]]
		static PropertyValue<T> fromJSON(const JSON& json, const T& defaultValue = T{})
		{
			if constexpr (std::is_enum_v<T>)
			{
				if (json.isString())
				{
					return PropertyValue<T>{ StringToEnum(json.getString(), defaultValue) };
				}
				else if (json.isObject() && json.contains(U"default"))
				{
					auto propertyValue = PropertyValue<T>
					{
						StringToEnum(json[U"default"].getString(), defaultValue),
						json.contains(U"hovered") ? StringToEnum(json[U"hovered"].getString(), defaultValue) : Optional<T>{ none },
						json.contains(U"pressed") ? StringToEnum(json[U"pressed"].getString(), defaultValue) : Optional<T>{ none },
						json.contains(U"disabled") ? StringToEnum(json[U"disabled"].getString(), defaultValue) : Optional<T>{ none },
						json.contains(U"smoothTime") ? json[U"smoothTime"].getOr<double>(0.0) : 0.0,
					};
					if (json.contains(U"selectedDefault"))
					{
						propertyValue.selectedDefaultValue = StringToEnum(json[U"selectedDefault"].getString(), defaultValue);
					}
					if (json.contains(U"selectedHovered"))
					{
						propertyValue.selectedHoveredValue = StringToEnum(json[U"selectedHovered"].getString(), defaultValue);
					}
					if (json.contains(U"selectedPressed"))
					{
						propertyValue.selectedPressedValue = StringToEnum(json[U"selectedPressed"].getString(), defaultValue);
					}
					if (json.contains(U"selectedDisabled"))
					{
						propertyValue.selectedDisabledValue = StringToEnum(json[U"selectedDisabled"].getString(), defaultValue);
					}
					return propertyValue;
				}
				return PropertyValue<T>{ defaultValue };
			}
			else if constexpr (HasFromJSON<T>)
			{
				if (json.isObject() && json.contains(U"default"))
				{
					auto propertyValue = PropertyValue<T>
					{
						T::fromJSON(json[U"default"], defaultValue),
						json.contains(U"hovered") ? T::fromJSON(json[U"hovered"], defaultValue) : Optional<T>{ none },
						json.contains(U"pressed") ? T::fromJSON(json[U"pressed"], defaultValue) : Optional<T>{ none },
						json.contains(U"disabled") ? T::fromJSON(json[U"disabled"], defaultValue) : Optional<T>{ none },
						json.contains(U"smoothTime") ? json[U"smoothTime"].getOr<double>(0.0) : 0.0,
					};
					if (json.contains(U"selectedDefault"))
					{
						propertyValue.selectedDefaultValue = T::fromJSON(json[U"selectedDefault"], defaultValue);
					}
					if (json.contains(U"selectedHovered"))
					{
						propertyValue.selectedHoveredValue = T::fromJSON(json[U"selectedHovered"], defaultValue);
					}
					if (json.contains(U"selectedPressed"))
					{
						propertyValue.selectedPressedValue = T::fromJSON(json[U"selectedPressed"], defaultValue);
					}
					if (json.contains(U"selectedDisabled"))
					{
						propertyValue.selectedDisabledValue = T::fromJSON(json[U"selectedDisabled"], defaultValue);
					}
					return propertyValue;
				}
				return PropertyValue<T>{ T::fromJSON(json, defaultValue) };
			}
			else
			{
				if (json.isObject() && json.contains(U"default"))
				{
					auto propertyValue = PropertyValue<T>
					{
						json[U"default"].getOr<T>(defaultValue),
						json.contains(U"hovered") ? json[U"hovered"].getOpt<T>() : Optional<T>{ none },
						json.contains(U"pressed") ? json[U"pressed"].getOpt<T>() : Optional<T>{ none },
						json.contains(U"disabled") ? json[U"disabled"].getOpt<T>() : Optional<T>{ none },
						json.contains(U"smoothTime") ? json[U"smoothTime"].getOr<double>(0.0) : 0.0,
					};
					if (json.contains(U"selectedDefault"))
					{
						propertyValue.selectedDefaultValue = json[U"selectedDefault"].getOpt<T>();
					}
					if (json.contains(U"selectedHovered"))
					{
						propertyValue.selectedHoveredValue = json[U"selectedHovered"].getOpt<T>();
					}
					if (json.contains(U"selectedPressed"))
					{
						propertyValue.selectedPressedValue = json[U"selectedPressed"].getOpt<T>();
					}
					if (json.contains(U"selectedDisabled"))
					{
						propertyValue.selectedDisabledValue = json[U"selectedDisabled"].getOpt<T>();
					}
					return propertyValue;
				}
				return PropertyValue<T>{ json.getOr<T>(defaultValue) };
			}
		}

		[[nodiscard]]
		PropertyValue<T> withDefault(const T& newDefaultValue) const
		{
			auto value = *this;
			value.defaultValue = newDefaultValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withDefault(const U& newDefaultValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			value.defaultValue = static_cast<T>(newDefaultValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withHover(const T& newHoveredValue) const
		{
			auto value = *this;
			value.hoveredValue = newHoveredValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withHover(const U& newHoveredValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			value.hoveredValue = static_cast<T>(newHoveredValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withPressed(const T& newPressedValue) const
		{
			auto value = *this;
			value.pressedValue = newPressedValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withPressed(const U& newPressedValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			value.pressedValue = static_cast<T>(newPressedValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withDisabled(const T& newDisabledValue) const
		{
			auto value = *this;
			value.disabledValue = newDisabledValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withDisabled(const U& newDisabledValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			value.disabledValue = static_cast<T>(newDisabledValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withSelectedDefault(const T& newSelectedDefaultValue) const
		{
			auto value = *this;
			value.selectedDefaultValue = newSelectedDefaultValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withSelectedDefault(const U& newSelectedDefaultValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			value.selectedDefaultValue = static_cast<T>(newSelectedDefaultValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withSelectedHover(const T& newSelectedHoveredValue) const
		{
			auto value = *this;
			value.selectedHoveredValue = newSelectedHoveredValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withSelectedHover(const U& newSelectedHoveredValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			value.selectedHoveredValue = static_cast<T>(newSelectedHoveredValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withSelectedPressed(const T& newSelectedPressedValue) const
		{
			auto value = *this;
			value.selectedPressedValue = newSelectedPressedValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withSelectedPressed(const U& newSelectedPressedValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			value.selectedPressedValue = static_cast<T>(newSelectedPressedValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withSelectedDisabled(const T& newSelectedDisabledValue) const
		{
			auto value = *this;
			value.selectedDisabledValue = newSelectedDisabledValue;
			return value;
		}

		template <class U>
		[[nodiscard]]
		PropertyValue<T> withSelectedDisabled(const U& newSelectedDisabledValue) const requires std::convertible_to<U, T>
		{
			auto value = *this;
			value.selectedDisabledValue = static_cast<T>(newSelectedDisabledValue);
			return value;
		}

		[[nodiscard]]
		PropertyValue<T> withSmoothTime(double newSmoothTime) const
		{
			auto value = *this;
			value.smoothTime = newSmoothTime;
			return value;
		}

		[[nodiscard]]
		String getValueString() const
		{
			const auto fnGetStr = [](const T& value)
				{
					if constexpr (std::is_enum_v<T>)
					{
						return EnumToString(value);
					}
					else
					{
						return Format(value);
					}
				};

			if (!hoveredValue && !pressedValue && !disabledValue && smoothTime == 0.0)
			{
				return fnGetStr(defaultValue);
			}

			// TODO: Hovered/Pressed/Disabled/Selectedの値を出力可能にする
			String str = fnGetStr(defaultValue);
			return str;
		}

		// TODO: hovered/pressed/disabled/selectedへの対応
		bool trySetValueString(StringView value)
		{
			if constexpr (std::is_enum_v<T>)
			{
				if (!EnumContains<T>(value))
				{
					return false;
				}
				defaultValue = StringToEnum(value, defaultValue);
			}
			else if constexpr (std::same_as<T, String>)
			{
				defaultValue = String{ value };
			}
			else
			{
				const auto resultOpt = ParseOpt<T>(value);
				if (!resultOpt)
				{
					return false;
				}
				defaultValue = *resultOpt;
			}
			hoveredValue = none;
			pressedValue = none;
			disabledValue = none;
			selectedDefaultValue = none;
			selectedHoveredValue = none;
			selectedPressedValue = none;
			selectedDisabledValue = none;
			smoothTime = 0.0;
			return true;
		}
	};
}
