#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "data/Identifier.h"
#include "registry/Registerable.h"

namespace Game3 {
	class Game;
	struct NamedRegistryBase;
	struct UnnamedRegistryBase;

	class Registry: public NamedRegisterable, public std::enable_shared_from_this<Registry> {
		protected:
			explicit Registry(Identifier identifier_): NamedRegisterable(std::move(identifier_)) {}

		public:
			Registry(): Registry(Identifier()) { throw std::logic_error("Cannot default-construct a Registry"); }
			virtual ~Registry() = default;

			template <typename T>
			std::shared_ptr<T> cast() {
				return std::dynamic_pointer_cast<T>(shared_from_this());
			}

			template <typename T>
			std::shared_ptr<const T> cast() const {
				return std::dynamic_pointer_cast<const T>(shared_from_this());
			}

			std::shared_ptr<NamedRegistryBase> toNamed() {
				return cast<NamedRegistryBase>();
			}

			std::shared_ptr<UnnamedRegistryBase> toUnnamed() {
				return cast<UnnamedRegistryBase>();
			}

		protected:
			size_t nextCounter = 0;
	};

	void from_json(const nlohmann::json &, Registry &);

	class IdentifierRegistry: public Registry {
		public:
			using Registry::Registry;

			std::set<Identifier> items;

			IdentifierRegistry & operator+=(Identifier item) {
				add(std::move(item));
				return *this;
			}

			template <typename S>
			void add() {
				add(S::ID());
			}

			void add(Identifier item) {
				items.insert(std::move(item));
			}

			template <typename S>
			const Identifier & contains() const {
				return items.contains(S::ID());
			}

			inline bool contains(const Identifier &id) {
				return items.contains(id);
			}

			inline void clear() {
				items.clear();
			}

			inline size_t size() const {
				return items.size();
			}
	};

	struct NamedRegistryBase: Registry {
		using Registry::Registry;
	};

	template <typename T>
	class NamedRegistry: public NamedRegistryBase {
#ifdef REGISTRY_ASSERTS
		static_assert(std::is_base_of_v<NamedRegisterable, T>);
#endif

		public:
			std::map<Identifier, std::shared_ptr<T>> items;
			std::vector<std::shared_ptr<T>> byCounter;

			using NamedRegistryBase::NamedRegistryBase;
			~NamedRegistry() override = default;

			inline NamedRegistry & operator+=(std::shared_ptr<T> item) {
				add(item);
				return *this;
			}

			inline NamedRegistry & operator+=(const std::pair<std::string, std::shared_ptr<T>> &pair) {
				add(pair.first, pair.second);
				return *this;
			}

			template <typename S>
			inline void add() {
				add(S::ID(), std::make_shared<S>());
			}

			inline void add(Identifier new_name, std::shared_ptr<T> new_item) {
				if (auto [iter, inserted] = items.try_emplace(new_name, std::move(new_item)); inserted) {
					iter->second->identifier = std::move(new_name);
					iter->second->registryID = nextCounter++;
					byCounter.push_back(iter->second);
				} else
					throw std::runtime_error("NamedRegistry " + identifier.str() + " already contains an item with name \"" + new_name.str() + '"');
			}

			inline void add(Identifier new_name, T &&new_item) {
				add(std::move(new_name), std::make_shared<T>(std::move(new_item)));
			}

			inline bool contains(const Identifier &id) const {
				return items.contains(id);
			}

			template <typename S>
			inline S & get() {
				return *items.at(S::ID())->template cast<S>();
			}

			template <typename S>
			inline const S & get() const {
				return *items.at(S::ID())->template cast<const S>();
			}

			inline std::shared_ptr<T> operator[](size_t counter) const {
				return at(counter);
			}

			inline std::shared_ptr<T> at(size_t counter) const {
				return byCounter.at(counter);
			}

			inline std::shared_ptr<T> operator[](const Identifier &id) const {
				return at(id);
			}

			inline std::shared_ptr<T> at(const Identifier &id) const {
				try {
					return items.at(id);
				} catch (const std::out_of_range &) {
					std::cerr << "\e[31mCouldn't find \"" + id.str() + "\" in registry " + identifier.str() << "\e[39m\n";
					return {};
				}
			}

			inline void clear() {
				items.clear();
				byCounter.clear();
				nextCounter = 0;
			}

			inline size_t size() const {
				return items.size();
			}
	};

	struct UnnamedRegistryBase: Registry {
		using Registry::Registry;

		virtual void add(const Game &, const nlohmann::json &) = 0;
	};

	template <typename T>
	class UnnamedRegistry: public UnnamedRegistryBase {
#ifdef REGISTRY_ASSERTS
		static_assert(std::is_base_of_v<Registerable, T>);
#endif

		public:
			std::unordered_set<std::shared_ptr<T>> items;
			std::vector<std::shared_ptr<T>> byCounter;

			using UnnamedRegistryBase::UnnamedRegistryBase;
			~UnnamedRegistry() override = default;

			inline UnnamedRegistry & operator+=(std::shared_ptr<T> item) {
				add(std::move(item));
				return *this;
			}

			inline void add(std::shared_ptr<T> item) {
				if (auto [iter, inserted] = items.insert(std::move(item)); inserted) {
					(*iter)->registryID = nextCounter++;
					byCounter.push_back(*iter);
				}
			}

			inline void add(T &&item) {
				add(std::make_shared<T>(std::move(item)));
			}

			bool addCarefully(std::shared_ptr<T> item) {
				for (const auto &existing: items)
					if (*existing == *item)
						return false;
				if (auto [iter, inserted] = items.insert(std::move(item)); inserted) {
					(*iter)->registryID = nextCounter++;
					byCounter.push_back(*iter);
					return true;
				}
				return false;
			}

			void add(const Game &, const nlohmann::json &) override {
				throw std::runtime_error("Adding from JSON unimplemented");
			}

			inline std::shared_ptr<T> operator[](size_t counter) const {
				return at(counter);
			}

			inline std::shared_ptr<T> at(size_t counter) const {
				return byCounter.at(counter);
			}

			inline void clear() {
				items.clear();
				byCounter.clear();
				nextCounter = 0;
			}

			inline size_t size() const {
				return items.size();
			}
	};

	template <typename T>
	class UnnamedJSONRegistry: public UnnamedRegistry<T> {
		public:
			using UnnamedRegistry<T>::UnnamedRegistry;

			void add(const Game &game, const nlohmann::json &json) override {
				// Why.
				static_cast<UnnamedRegistry<T> *>(this)->add(T::fromJSON(game, json));
			}
	};
}
