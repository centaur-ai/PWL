#ifndef SET_GRAPH_H_
#define SET_GRAPH_H_

#include <core/array.h>
#include <core/map.h>
#include <stdint.h>
#include <set>

using namespace core;


/* forward declarations */

template<typename Formula> struct set_reasoning;

template<int MinPriority, typename Formula>
bool find_largest_disjoint_clique_with_set(
		const set_reasoning<Formula>&, unsigned int,
		unsigned int*&, unsigned int&, unsigned int&);
template<int MinPriority, typename Formula>
bool find_largest_disjoint_clique_with_set(
		const set_reasoning<Formula>&, unsigned int, unsigned int,
		unsigned int*&, unsigned int&, unsigned int&);

struct set_vertex {
	array<unsigned int> parents;
	array<unsigned int> children;

	static inline void free(set_vertex& vertex) {
		core::free(vertex.parents);
		core::free(vertex.children);
	}
};

inline bool init(set_vertex& vertex) {
	if (!array_init(vertex.parents, 4)) {
		return false;
	} else if (!array_init(vertex.children, 4)) {
		core::free(vertex.parents);
		return false;
	}
	return true;
}

struct set_cover {
	array<unsigned int> sets;
	unsigned int covered_set;
};

struct set_graph {
	set_vertex* vertices;

	set_graph(unsigned int initial_capacity) {
		vertices = (set_vertex*) malloc(sizeof(set_vertex) * initial_capacity);
		if (vertices == NULL) exit(EXIT_FAILURE);
	}

	~set_graph() {
		free(vertices);
	}

	inline bool resize(unsigned int new_capacity) {
		return core::resize(vertices, new_capacity);
	}

	inline bool new_set(unsigned int vertex_id) {
		return init(vertices[vertex_id]);
	}

	template<bool CleanupEdges>
	inline void free_set(unsigned int vertex_id) {
		if (CleanupEdges) {
			for (unsigned int parent : vertices[vertex_id].parents)
				remove_edge(parent, vertex_id);
			for (unsigned int child : vertices[vertex_id].children)
				remove_edge(vertex_id, child);
		}
		core::free(vertices[vertex_id]);
	}

	inline bool add_edge(unsigned int parent, unsigned int child) {
		if (!vertices[parent].children.add(child)) {
			return false;
		} else if (!vertices[child].parents.add(parent)) {
			vertices[parent].children.length--;
			return false;
		}
		return true;
	}

	inline void remove_edge(unsigned int parent, unsigned int child) {
		unsigned int index = vertices[parent].children.index_of(child);
#if !defined(NDEBUG)
		if (index == vertices[parent].children.length)
			fprintf(stderr, "set_graph.remove_edge WARNING: The set %u is not in `vertices[%u].children`.\n", child, parent);
#endif
		vertices[parent].children.remove(index);
		index = vertices[child].parents.index_of(parent);
#if !defined(NDEBUG)
		if (index == vertices[child].parents.length)
			fprintf(stderr, "set_graph.remove_edge WARNING: The set %u is not in `vertices[%u].children`.\n", parent, child);
#endif
		vertices[child].parents.remove(index);
	}
};

template<typename SetGraph>
bool get_ancestors(
		const SetGraph& graph, unsigned int vertex,
		array_map<unsigned int, unsigned int>& ancestors)
{
	if (!ancestors.ensure_capacity(ancestors.size + 1)) return false;
	unsigned int index = ancestors.index_of(vertex);
	if (index < ancestors.size) {
		ancestors.values[index]++;
		return true;
	}
	ancestors.keys[index] = vertex;
	ancestors.values[index] = 0;
	ancestors.size++;

	array<unsigned int> stack(8);
	stack.add(vertex);
	while (stack.length > 0) {
		unsigned int v = stack.pop();
		for (unsigned int parent : graph.vertices[v].parents) {
			if (!ancestors.ensure_capacity(ancestors.size + 1)) return false;
			unsigned int index = ancestors.index_of(parent);
			if (index < ancestors.size) {
				ancestors.values[index]++;
			} else {
				ancestors.keys[index] = parent;
				ancestors.values[index] = 1;
				ancestors.size++;
				if (!stack.add(parent)) return false;
			}
		}
	}
	return true;
}

template<typename SetGraph>
bool get_descendants(
		const SetGraph& graph, unsigned int vertex,
		array_map<unsigned int, unsigned int>& descendants)
{
	if (!descendants.ensure_capacity(descendants.size + 1)) return false;
	unsigned int index = descendants.index_of(vertex);
	if (index < descendants.size) {
		descendants.values[index]++;
		return true;
	}
	descendants.keys[index] = vertex;
	descendants.values[index] = 0;
	descendants.size++;

	array<unsigned int> stack(8);
	stack.add(vertex);
	while (stack.length > 0) {
		unsigned int v = stack.pop();
		for (unsigned int child : graph.vertices[v].children) {
			if (!descendants.ensure_capacity(descendants.size + 1)) return false;
			unsigned int index = descendants.index_of(child);
			if (index < descendants.size) {
				descendants.values[index]++;
			} else {
				descendants.keys[index] = child;
				descendants.values[index] = 1;
				descendants.size++;
				if (!stack.add(child)) return false;
			}
		}
	}
	return true;
}

template<typename Formula>
struct set_info {
	unsigned int set_size;
	bool fixed_set_size;
	Formula* set_formula;

	static inline void free(set_info& info) {
		core::free(*info.set_formula);
		if (info.set_formula->reference_count == 0)
			core::free(info.set_formula);
		info.set_formula = NULL;
	}
};

template<typename Formula>
inline bool init(set_info<Formula>& info, unsigned int set_size, Formula* set_formula) {
	info.set_size = set_size;
	info.fixed_set_size = false;
	info.set_formula = set_formula;
	set_formula->reference_count++;
	return true;
}

template<typename Formula>
struct set_reasoning {
	set_graph extensional_graph;
	set_graph intensional_graph;
	set_info<Formula>* sets;

	unsigned int capacity;
	unsigned int set_count;

	hash_map<Formula, unsigned int> set_ids;

	set_reasoning() : extensional_graph(1024), intensional_graph(1024), capacity(1024), set_count(0), set_ids(2048)
	{
		sets = (set_info<Formula>*) malloc(sizeof(set_info<Formula>) * capacity);
		if (sets == NULL) exit(EXIT_FAILURE);
		for (unsigned int i = 1; i < capacity; i++)
			sets[i].set_formula = NULL;

		unsigned int empty_set_id;
		Formula* empty = Formula::new_false();
		if (empty == NULL
		 || !get_set_id(empty, empty_set_id))
			exit(EXIT_FAILURE);
		free(*empty); if (empty->reference_count == 0) free(empty);
		sets[empty_set_id].set_size = 0;
		sets[empty_set_id].fixed_set_size = true;
	}

	~set_reasoning() {
		for (unsigned int i = 1; i < set_count + 1; i++) {
			if (sets[i].set_formula != NULL) {
				extensional_graph.free_set<false>(i);
				intensional_graph.free_set<false>(i);
				core::free(sets[i]);
			}
		} for (auto entry : set_ids)
			core::free(entry.key);
		core::free(sets);
	}

	bool ensure_capacity(unsigned int new_length) {
		if (new_length <= capacity) return true;
		unsigned int new_capacity = capacity;
		expand_capacity(new_capacity, new_length);

		if (!resize(sets, new_capacity) || !extensional_graph.resize(new_capacity) || !intensional_graph.resize(new_capacity))
			return false;
		for (unsigned int i = capacity; i < new_capacity; i++)
			sets[i].set_formula = NULL; /* we use this field to mark which vertices are free */
		capacity = new_capacity;
		return true;
	}

	unsigned int get_next_free_set() const {
		for (unsigned int i = 1; i < capacity; i++)
			if (sets[i].set_formula == NULL) return i;
		return capacity;
	}

	bool is_freeable(unsigned int set_id) const {
		return !sets[set_id].fixed_set_size
			&& extensional_graph.vertices[set_id].children.length == 0
			&& extensional_graph.vertices[set_id].parents.length == 0;
	}

	inline bool compute_new_set_size(unsigned int& out,
			unsigned int min_set_size, unsigned int max_set_size)
	{
		out = (max_set_size == UINT_MAX) ? (min_set_size + 10) : ((min_set_size + max_set_size) / 2);
		return true;
	}

	bool new_set(Formula* set_formula, unsigned int& set_id)
	{
		set_id = get_next_free_set();
		if (!ensure_capacity(set_id + 1)
		 || !extensional_graph.new_set(set_id))
		{
			return false;
		} else if (!intensional_graph.new_set(set_id)) {
			extensional_graph.free_set<true>(set_id);
			return false;
		}

		/* initialize all intensional set relations */
		array<unsigned int> supersets(8), subsets(8);
		for (unsigned int i = 1; i < set_count + 1; i++) {
			if (sets[i].set_formula == NULL) continue;
			if (is_subset(sets[i].set_formula, set_formula)) {
				if (!subsets.add(i)) {
					intensional_graph.free_set<true>(set_id);
					extensional_graph.free_set<true>(set_id);
					return false;
				}
			} else if (is_subset(set_formula, sets[i].set_formula)) {
				if (!supersets.add(i)) {
					intensional_graph.free_set<true>(set_id);
					extensional_graph.free_set<true>(set_id);
					return false;
				}
			}
		}

		/* compute the ancestors of `new_set` and their degrees (in the subgraph of the ancestors) */
		array_map<unsigned int, unsigned int> ancestors(8);
		for (unsigned int superset : supersets) {
			if (!get_ancestors(intensional_graph, superset, ancestors)) {
				intensional_graph.free_set<true>(set_id);
				extensional_graph.free_set<true>(set_id);
				return false;
			}
		}
		/* remove ancestors with non-zero out-degree */
		unsigned int next = 0;
		for (unsigned int i = 0; i < ancestors.size; i++) {
			if (ancestors.values[i] == 0) {
				ancestors.keys[next] = ancestors.keys[i];
				ancestors.values[next++] = 0;
			}
		}
		ancestors.size = next;
		/* the vertices with out-degree 0 in `ancestors` are the immediate parents of the new vertex `set_id` */
		for (const auto& entry : ancestors) {
			if (!intensional_graph.add_edge(entry.key, set_id)) {
				intensional_graph.free_set<true>(set_id);
				extensional_graph.free_set<true>(set_id);
				return false;
			}
		}

		/* compute the descendants of `new_set` and their degrees (in the subgraph of the descendants) */
		array_map<unsigned int, unsigned int> descendants(8);
		for (unsigned int subset : subsets) {
			if (!get_descendants(intensional_graph, subset, descendants)) {
				intensional_graph.free_set<true>(set_id);
				extensional_graph.free_set<true>(set_id);
				return false;
			}
		}
		/* remove descendants with non-zero in-degree */
		next = 0;
		for (unsigned int i = 0; i < descendants.size; i++) {
			if (descendants.values[i] == 0) {
				descendants.keys[next] = descendants.keys[i];
				descendants.values[next++] = 0;
			}
		}
		descendants.size = next;
		/* the vertices with in-degree 0 in `descendants` are the immediate children of the new vertex `set_id` */
		for (const auto& entry : descendants) {
			if (!intensional_graph.add_edge(set_id, entry.key)) {
				intensional_graph.free_set<true>(set_id);
				extensional_graph.free_set<true>(set_id);
				return false;
			}
		}

		/* initialize the set_info structure and the set size */
		if (!init(sets[set_id], 0, set_formula)) {
			intensional_graph.free_set<true>(set_id);
			extensional_graph.free_set<true>(set_id);
			return false;
		}

		/* remove edges connecting ancestors to descendants */
		for (const auto& immediate_ancestor : ancestors) {
			for (const auto& immediate_descendant : descendants) {
				/* remove the edge from `parent` to `child` if it exists */
				if (intensional_graph.vertices[immediate_ancestor.key].children.contains(immediate_descendant.key))
					intensional_graph.remove_edge(immediate_ancestor.key, immediate_descendant.key);
			}
		}

		/* compute the upper bound and lower bound on the size of this new set */
		unsigned int min_set_size, max_set_size, initial_set_size;
		if (!get_size_lower_bound(set_id, min_set_size)
		 || !get_size_upper_bound(set_id, max_set_size)
		 || !compute_new_set_size(initial_set_size, min_set_size, max_set_size))
		{
			return free_set(set_id);
		}
		sets[set_id].set_size = initial_set_size;

		if (set_id == set_count + 1)
			set_count++;
		return true;
	}

	bool free_set(unsigned int set_id)
	{
		intensional_graph.free_set<true>(set_id);
		extensional_graph.free_set<true>(set_id);
		core::free(sets[set_id]);

		for (unsigned int parent : intensional_graph.vertices[set_id].parents) {
			array_map<unsigned int, unsigned int> descendants(8);
			if (!get_descendants(intensional_graph, parent, descendants))
				return false;
			for (unsigned int child : intensional_graph.vertices[set_id].children) {
				/* first check if there is already a path from `parent` to `child` */
				if (descendants.contains(child)) continue;
				if (!intensional_graph.add_edge(parent, child)) return false;
			}
		}
		return true;
	}

	inline bool free_set(Formula* formula, unsigned int set_id)
	{
		if (!free_set(set_id)) return false;

		bool contains; unsigned int bucket;
		set_ids.get(*formula, contains, bucket);
		core::free(set_ids.table.keys[bucket]);
		core::set_empty(set_ids.table.keys[bucket]);
		return true;
	}

	inline bool get_set_id(Formula* formula, unsigned int& set_id) {
		bool contains; unsigned int bucket;
		set_id = set_ids.get(*formula, contains, bucket);
		if (!contains) {
			if (!init(set_ids.table.keys[bucket], *formula)) {
				return false;
			} else if (!new_set(formula, set_id)) {
				core::free(set_ids.table.keys[bucket]);
				core::set_empty(set_ids.table.keys[bucket]);
				return false;
			}
			set_ids.values[bucket] = set_id;
			set_ids.table.size++;
		}
		return true;
	}

	bool add_subset_relation(Formula* antecedent, Formula* consequent)
	{
		if (!set_ids.check_size(set_ids.table.size + 2)) return false;

		unsigned int antecedent_set, consequent_set;
		if (!get_set_id(antecedent, antecedent_set)) {
			return false;
		} else if (!get_set_id(consequent, consequent_set)) {
			if (is_freeable(antecedent_set)) free_set(antecedent, antecedent_set);
			return false;
		}

		if (consequent_set != antecedent_set && !extensional_graph.add_edge(consequent_set, antecedent_set)) {
			/* if either the antecedent or consequent sets have no references, free them */
			if (is_freeable(consequent_set)) free_set(consequent, consequent_set);
			if (is_freeable(antecedent_set)) free_set(antecedent, antecedent_set);
			return false;
		}

		/* TODO: we have to check that the new edge does not create any inconsistencies */
		unsigned int* clique; unsigned int clique_count; unsigned int ancestor_of_clique;
		find_largest_disjoint_clique_with_set<0>(*this, antecedent_set, consequent_set, clique, clique_count, ancestor_of_clique);
		return true;
	}

	void remove_subset_relation(Formula* antecedent, Formula* consequent)
	{
#if !defined(NDEBUG)
		bool contains;
		unsigned int antecedent_set = set_ids.get(*antecedent, contains);
		if (!contains) fprintf(stderr, "set_reasoning.remove_subset_relation WARNING: No such set for given antecedent.\n");
		unsigned int consequent_set = set_ids.get(*consequent, contains);
		if (!contains) fprintf(stderr, "set_reasoning.remove_subset_relation WARNING: No such set for given consequent.\n");
#else
		unsigned int antecedent_set = set_ids.get(*antecedent);
		unsigned int consequent_set = set_ids.get(*consequent);
#endif

		if (consequent_set != antecedent_set)
			extensional_graph.remove_edge(consequent_set, antecedent_set);

		/* if either the antecedent or consequent sets have no references, free them */
		if (is_freeable(consequent_set)) free_set(consequent, consequent_set);
		if (is_freeable(antecedent_set)) free_set(antecedent, antecedent_set);
	}

	bool fix_size(const Formula* formula)
	{
		unsigned int set_id;
		if (!get_set_id(formula, set_id))
			return false;
		sets[set_id].fixed_set_size = true;
		return true;
	}

	bool unfix_size(const Formula* formula)
	{
		unsigned int set_id;
		if (!get_set_id(formula, set_id))
			return false;
		sets[set_id].fixed_set_size = false;
		return true;
	}

	/* NOTE: this function assumes that `new_size` does not violate the bounds
	   given by the functions `get_size_lower_bound` and `get_size_upper_bound` */
	bool set_size(Formula* formula, unsigned int new_size) {
		unsigned int set_id;
		if (!get_set_id(formula, set_id))
			return false;
		sets[set_id].set_size = new_size;
		return true;
	}

	bool force_set_size(Formula* formula, unsigned int new_size) {
		unsigned int set_id;
		if (!get_set_id(formula, set_id))
			return false;
		if (new_size > sets[set_id].set_size) {
			while (true) {
				/* compute the upper bound on the size of this set; if the new size
				   violates this bound, change the sizes of other sets to increase the bound */
				unsigned int* clique = NULL; unsigned int clique_count; unsigned int ancestor_of_clique;
				if (!find_largest_disjoint_clique_with_set<INT_MIN>(*this, set_id, clique, clique_count, ancestor_of_clique))
					return false;
				if (clique == NULL)
					break; /* the upper bound is infinite */

				unsigned int index = index_of(set_id, clique, clique_count);
				clique[index] = clique[--clique_count];

				unsigned int clique_size = 0;
				for (unsigned int i = 0; i < clique_count; i++)
					clique_size += sets[clique[i]].set_size;
				if (clique_size + new_size <= sets[ancestor_of_clique].set_size)
					break;

				/* the upper bound is violated */
				/* TODO: implement this */
				fprintf(stderr, "set_reasoning.force_set_size ERROR: Handling upper bound violation is not implemented.\n");
				exit(EXIT_FAILURE);
			}
			sets[set_id].set_size = new_size;
		} else if (new_size < sets[set_id].set_size) {
			while (true) {
				/* compute the lower bound on the size of this set; if the new size
				   violates this bound, change the sizes of other sets to increase the bound */
				unsigned int* clique = NULL; unsigned int clique_count;
				if (!find_largest_disjoint_subset_clique(*this, set_id, clique, clique_count))
					return false;
				if (clique == NULL)
					break; /* the lower bound is zero */

				unsigned int clique_size = 0;
				for (unsigned int i = 0; i < clique_count; i++)
					clique_size += sets[clique[i]].set_size;
				if (clique_size <= new_size)
					break;

				/* the lower bound is violated */
				/* TODO: implement this */
				fprintf(stderr, "set_reasoning.force_set_size ERROR: Handling lower bound violation is not implemented.\n");
				exit(EXIT_FAILURE);
			}
			sets[set_id].set_size = new_size;
		}
		return true;
	}

	bool get_size_lower_bound(unsigned int set_id, unsigned int& lower_bound) const
	{
		unsigned int* clique = NULL; unsigned int clique_count;
		if (!find_largest_disjoint_subset_clique(*this, set_id, clique, clique_count))
			return false;
		if (clique == NULL) {
			lower_bound = 0;
			return true;
		}

		lower_bound = 0;
		for (unsigned int i = 0; i < clique_count; i++)
			lower_bound += sets[clique[i]].set_size;
		free(clique);
		return true;
	}

	bool get_size_upper_bound(unsigned int set_id, unsigned int& upper_bound) const
	{
		unsigned int* clique = NULL; unsigned int clique_count; unsigned int ancestor_of_clique;
		if (!find_largest_disjoint_clique_with_set<INT_MIN>(*this, set_id, clique, clique_count, ancestor_of_clique))
			return false;
		if (clique == NULL) {
			upper_bound = UINT_MAX; /* the upper bound is infinite */
			return true;
		}

		unsigned int index = index_of(set_id, clique, clique_count);
		clique[index] = clique[--clique_count];

		upper_bound = sets[ancestor_of_clique].set_size;
		for (unsigned int i = 0; i < clique_count; i++)
			upper_bound -= sets[clique[i]].set_size;
		free(clique);
		return true;
	}

	inline bool are_disjoint(unsigned int first_set, unsigned int second_set) const {
		Formula* intersection = intersect(sets[first_set].set_formula, sets[second_set].set_formula);

		bool contains;
		unsigned int id = set_ids.get(*intersection, contains);
		free(*intersection); if (intersection->reference_count == 0) free(intersection);
		if (!contains) return false;
		return sets[id].set_size == 0;
	}
};

struct clique_search_state {
	unsigned int* clique;
	unsigned int clique_count;
	unsigned int* neighborhood;
	unsigned int neighborhood_count;
	unsigned int* X;
	unsigned int X_count;
	unsigned int next_set;
	int priority;

	inline int get_priority() const {
		return priority;
	}

	static inline void free(clique_search_state& state) {
		core::free(state.clique);
		core::free(state.neighborhood);
		core::free(state.X);
	}
};

template<typename Formula>
inline bool init(clique_search_state& state,
		const set_reasoning<Formula>& sets,
		const unsigned int* clique, unsigned int clique_count,
		const unsigned int* neighborhood, unsigned int neighborhood_count,
		const unsigned int* X, unsigned int X_count,
		unsigned int next_set_to_expand, int priority)
{
	state.clique = (unsigned int*) malloc(max((size_t) 1, sizeof(unsigned int) * clique_count));
	if (state.clique == NULL) {
		fprintf(stderr, "init ERROR: Insufficient memory for `clique_search_state.clique`.");
		return false;
	}
	for (unsigned int i = 0; i < clique_count; i++)
		state.clique[i] = clique[i];
	state.clique_count = clique_count;

	state.neighborhood = (unsigned int*) malloc(max((size_t) 1, sizeof(unsigned int) * neighborhood_count));
	if (state.neighborhood == NULL) {
		fprintf(stderr, "init ERROR: Insufficient memory for `clique_search_state.neighborhood`.");
		core::free(state.clique); return false;
	}
	for (unsigned int i = 0; i < neighborhood_count; i++)
		state.neighborhood[i] = neighborhood[i];
	state.neighborhood_count = neighborhood_count;

	state.X = (unsigned int*) malloc(max((size_t) 1, sizeof(unsigned int) * X_count));
	if (state.X == NULL) {
		fprintf(stderr, "init ERROR: Insufficient memory for `clique_search_state.X`.");
		core::free(state.clique); core::free(state.neighborhood); return false;
	}
	for (unsigned int i = 0; i < X_count; i++)
		state.X[i] = X[i];
	state.X_count = X_count;

	state.next_set = next_set_to_expand;
	state.priority = priority;
	return true;
}

template<typename Stream>
bool print(const clique_search_state& state, Stream& out) {
	return print("  clique: ", out) && print<unsigned int, '{', '}'>(state.clique, state.clique_count, out) && print('\n', out)
		&& print("  neighborhood: ", out) && print<unsigned int, '{', '}'>(state.neighborhood, state.neighborhood_count, out) && print('\n', out)
		&& print("  X: ", out) && print<unsigned int, '{', '}'>(state.X, state.X_count, out) && print('\n', out)
		&& print("  next_set: ", out) && print(state.next_set, out) && print('\n', out);
}

struct ancestor_clique_search_state {
	clique_search_state clique_state;
	unsigned int ancestor;

	inline int get_priority() const {
		return clique_state.priority;
	}

	static inline void free(ancestor_clique_search_state& state) {
		core::free(state.clique_state);
	}
};

template<typename Formula>
inline bool init(ancestor_clique_search_state& state,
		const set_reasoning<Formula>& sets,
		const unsigned int* clique, unsigned int clique_count,
		const unsigned int* neighborhood, unsigned int neighborhood_count,
		const unsigned int* X, unsigned int X_count,
		unsigned int next_set_to_expand, int neighborhood_size,
		unsigned int ancestor)
{
	if (!init(state.clique_state, sets, clique, clique_count, neighborhood, neighborhood_count,
			X, X_count, next_set_to_expand, neighborhood_size - sets.sets[ancestor].set_size))
		return false;
	state.ancestor = ancestor;
	return true;
}

template<typename StateData>
struct search_state {
	StateData* state;

	static inline void free(search_state<StateData>& state) {
		core::free(*state.state);
		core::free(state.state);
	}
};

template<typename StateData>
inline bool operator < (const search_state<StateData>& first, const search_state<StateData>& second) {
	return first.state->get_priority() < second.state->get_priority();
}

template<typename StateData, typename Stream>
inline bool print(const search_state<StateData>& state, Stream& out) {
	return print(state.state, out)
		&& print("  priority: ", out) && print(state.priority, out) && print('\n', out);
}

template<typename StateData>
struct search_queue {
	std::multiset<search_state<StateData>> queue;
	int last_priority;

	search_queue() : last_priority(INT_MAX) { }

	~search_queue() {
		for (search_state<StateData> state : queue)
			core::free(state);
	}

	inline bool is_empty() const {
		return queue.empty();
	}

	inline unsigned int size() const {
		return queue.size();
	}

	inline void push(const search_state<StateData>& state) {
//#if !defined(NDEBUG)
		if (state.state->get_priority() > last_priority)
			fprintf(stderr, "search_queue.push WARNING: Search is not monotonic.\n");
//#endif
		queue.insert(state);
	}

	inline search_state<StateData> pop(unsigned int iteration) {
		auto last = queue.cend(); last--;
		search_state<StateData> state = *last;
		queue.erase(last);

		last_priority = state.state->get_priority();
		return state;
	}

	inline int priority() const {
		auto last = queue.cend(); last--;
		search_state<StateData> state = *last;
		return state.state->get_priority();
	}
};

template<typename Formula>
const hash_set<unsigned int>& get_descendants(const set_reasoning<Formula>& sets,
		hash_map<unsigned int, hash_set<unsigned int>>& descendants, unsigned int root)
{
	if (!descendants.check_size()) exit(EXIT_FAILURE);

	bool contains; unsigned int bucket;
	hash_set<unsigned int>& value = descendants.get(root, contains, bucket);
	if (!contains) {
		if (!hash_set_init(value, 8)) exit(EXIT_FAILURE);
		value.keys[value.index_of(root)] = root;
		value.size++;
		for (unsigned int child : sets.extensional_graph.vertices[root].children)
			if (!value.add_all(get_descendants(sets, descendants, child))) exit(EXIT_FAILURE);
		for (unsigned int child : sets.intensional_graph.vertices[root].children)
			if (!value.add_all(get_descendants(sets, descendants, child))) exit(EXIT_FAILURE);
		descendants.table.keys[bucket] = root;
		descendants.table.size++;
	}
	return value;
}

template<typename Formula>
bool has_descendant(const set_reasoning<Formula>& sets,
		hash_map<unsigned int, hash_set<unsigned int>>& descendants,
		unsigned int root, unsigned int vertex)
{
	return get_descendants(sets, descendants, root).contains(vertex);
}

template<typename Formula>
bool expand_clique_search_state(
		const set_reasoning<Formula>& sets,
		array<unsigned int>& neighborhood,
		const unsigned int* X, const unsigned int X_count,
		hash_map<unsigned int, hash_set<unsigned int>>& descendants,
		hash_set<unsigned int>& visited,
		unsigned int set_to_expand,
		unsigned int root)
{
	if (!visited.add(root)) return false;

	if (sets.are_disjoint(set_to_expand, root)) {
		for (unsigned int i = 0; i < neighborhood.length; i++)
			if (has_descendant(sets, descendants, neighborhood[i], root)) { neighborhood.remove(i); i--; }

		if (!neighborhood.ensure_capacity(neighborhood.length + 1))
			return false;

#if !defined(NDEBUG)
		if (neighborhood.contains(root))
			fprintf(stderr, "expand_clique_search_state WARNING: `neighborhood` contains `root`.\n");
#endif

		neighborhood[neighborhood.length++] = root;
	} else {
		for (unsigned int child : sets.extensional_graph.vertices[root].children) {
			if (sets.sets[child].set_size == 0) continue;
			bool was_visited = visited.contains(child);
			for (unsigned int i = 0; !was_visited && i < X_count; i++)
				if (has_descendant(sets, descendants, X[i], child)) was_visited = true;
			for (unsigned int neighbor : neighborhood)
				if (has_descendant(sets, descendants, neighbor, child)) was_visited = true;
			if (was_visited) continue;
			if (!expand_clique_search_state(sets, neighborhood, X, X_count, descendants, visited, set_to_expand, child)) return false;
		} for (unsigned int child : sets.intensional_graph.vertices[root].children) {
			if (sets.sets[child].set_size == 0) continue;
			bool was_visited = visited.contains(child);
			for (unsigned int i = 0; !was_visited && i < X_count; i++)
				if (has_descendant(sets, descendants, X[i], child)) was_visited = true;
			for (unsigned int neighbor : neighborhood)
				if (has_descendant(sets, descendants, neighbor, child)) was_visited = true;
			if (was_visited) continue;
			if (!expand_clique_search_state(sets, neighborhood, X, X_count, descendants, visited, set_to_expand, child)) return false;
		}
	}
	return true;
}

template<bool RecurseChildren, bool TestCompletion, bool ReturnOnCompletion,
	int MinPriority, typename StateData, typename Formula, typename... StateArgs>
bool process_clique_search_state(
		search_queue<StateData>& queue,
		const set_reasoning<Formula>& sets,
		const clique_search_state& state,
		hash_map<unsigned int, hash_set<unsigned int>>& descendants,
		unsigned int*& clique, unsigned int& clique_count,
		StateArgs&&... state_args)
{
	unsigned int* new_clique = (unsigned int*) malloc(sizeof(unsigned int) * (state.clique_count + 1));
	if (new_clique == NULL) {
		fprintf(stderr, "process_clique_search_state ERROR: Insufficient memory for `new_clique`.\n");
		return false;
	}
	for (unsigned int i = 0; i < state.clique_count; i++)
		new_clique[i] = state.clique[i];
	new_clique[state.clique_count] = state.next_set;

	array<unsigned int> neighborhood(16);
	hash_set<unsigned int> visited(16);
	for (unsigned int i = 0; i < state.X_count; i++) {
		if (!expand_clique_search_state(sets, neighborhood, neighborhood.data, neighborhood.length, descendants, visited, state.next_set, state.X[i])) {
			free(new_clique);
			return false;
		}
	}
	unsigned int new_X_count = neighborhood.length;
	for (unsigned int i = 0; i < state.neighborhood_count; i++) {
		if (!expand_clique_search_state(sets, neighborhood, neighborhood.data, new_X_count, descendants, visited, state.next_set, state.neighborhood[i])) {
			free(new_clique);
			return false;
		}
	}

	int priority = sets.sets[state.next_set].set_size;
	for (unsigned int i = 0; i < state.clique_count; i++)
		priority += sets.sets[state.clique[i]].set_size;
	for (unsigned int i = new_X_count; i < neighborhood.length; i++)
		priority += sets.sets[neighborhood[i]].set_size;

	for (unsigned int i = new_X_count; i < neighborhood.length; i++) {
		if (priority < MinPriority) break;
		search_state<StateData> new_state;
		new_state.state = (StateData*) malloc(sizeof(StateData));
		if (new_state.state == NULL) {
			free(new_clique);
			return false;
		} else if (!init(*new_state.state, sets, new_clique, state.clique_count + 1,
				neighborhood.data + i + 1, neighborhood.length - i - 1,
				neighborhood.data, i, neighborhood[i], priority, std::forward<StateArgs>(state_args)...))
		{
			free(new_state.state); free(new_clique);
			return false;
		}

		queue.push(new_state);
		priority -= sets.sets[neighborhood[i]].set_size;
	}

	if (TestCompletion && state.neighborhood_count == 0 && state.X_count == 0) {
		/* `expand_clique_search_state` did not add any states to the queue, meaning this clique is maximal */
		clique = new_clique;
		clique_count = state.clique_count + 1;
		if (ReturnOnCompletion) return true;
	} else {
		free(new_clique);
	}

	if (RecurseChildren) {
		unsigned int old_neighborhood_length = neighborhood.length;
		const array<unsigned int>& extensional_children = sets.extensional_graph.vertices[state.next_set].children;
		const array<unsigned int>& intensional_children = sets.intensional_graph.vertices[state.next_set].children;
		if (!neighborhood.append(extensional_children.data, extensional_children.length))
			return false;
		for (unsigned int i = 0; i < intensional_children.length; i++) {
			if (extensional_children.contains(intensional_children[i])) continue;
			if (!neighborhood.add(intensional_children[i])) return false;
		}

		priority -= sets.sets[state.next_set].set_size;		
		for (unsigned int i = old_neighborhood_length; i < neighborhood.length; i++)
			priority += sets.sets[neighborhood[i]].set_size;

		for (unsigned int i = old_neighborhood_length; i < neighborhood.length; i++) {
			if (priority < MinPriority) break;
			unsigned int child = neighborhood[i];
			if (sets.sets[child].set_size == 0) continue;
			search_state<StateData> new_state;
			new_state.state = (StateData*) malloc(sizeof(StateData));
			if (new_state.state == NULL) {
				return false;
			} else if (!init(*new_state.state, sets, state.clique, state.clique_count,
					neighborhood.data + i + 1, neighborhood.length - i - 1,
					neighborhood.data, i, child, priority, std::forward<StateArgs>(state_args)...))
			{
				free(new_state.state);
				return false;
			}
			queue.push(new_state);
			priority -= sets.sets[child].set_size;
		}
	}

	return true;
}

template<typename Formula>
bool find_largest_disjoint_subset_clique(
		const set_reasoning<Formula>& sets, unsigned int root,
		unsigned int*& clique, unsigned int& clique_count)
{
	search_queue<clique_search_state> queue;
	hash_map<unsigned int, hash_set<unsigned int>> descendants(64);

	clique_search_state initial_state;
	if (!init(initial_state, sets, NULL, 0, NULL, 0, NULL, 0, root, INT_MAX)) {
		return false;
	} else if (!process_clique_search_state<true, false, false, INT_MIN>(queue, sets, initial_state, descendants, clique, clique_count)) {
		free(initial_state);
		for (auto entry : descendants) free(entry.value);
		return false;
	}
	free(initial_state);

	bool success = true;
	clique = NULL; clique_count = 0;
	for (unsigned int iteration = 0; success && !queue.is_empty(); iteration++) {
		search_state<clique_search_state> next = queue.pop(iteration);
		success = process_clique_search_state<true, true, true, INT_MIN>(queue, sets, *next.state, descendants, clique, clique_count);
		free(next);

		if (clique != NULL) break;
	}

	for (auto entry : descendants) free(entry.value);
	return success;
}

inline bool add_non_ancestor_neighbor(
		unsigned int child, bool& changed, array<unsigned int>& neighborhood,
		hash_map<unsigned int, array<unsigned int>>& non_ancestor_neighborhood)
{
	bool contains;
	array<unsigned int>& child_neighborhood = non_ancestor_neighborhood.get(child, contains);
	if (contains) {
		for (unsigned int child_neighbor : child_neighborhood) {
			if (neighborhood.contains(child_neighbor)) continue;
			if (!neighborhood.add(child_neighbor)) return false;
			changed = true;
		}
	} else {
		if (!neighborhood.add(child)) return false;
		changed = true;
	}
	return true;
}

template<typename Formula>
inline bool add_non_ancestor_neighbors(
		const set_reasoning<Formula>& sets, unsigned int node, bool& changed,
		hash_map<unsigned int, array<unsigned int>>& non_ancestor_neighborhood)
{
	array<unsigned int>& neighborhood = non_ancestor_neighborhood.get(node);
	for (unsigned int child : sets.extensional_graph.vertices[node].children)
		if (!add_non_ancestor_neighbor(child, changed, neighborhood, non_ancestor_neighborhood)) return false;
	for (unsigned int child : sets.intensional_graph.vertices[node].children)
		if (!add_non_ancestor_neighbor(child, changed, neighborhood, non_ancestor_neighborhood)) return false;
	return true;
}

struct default_parents {
	template<typename Formula, typename Function>
	bool for_each_parent(const set_reasoning<Formula>& sets, unsigned int set, Function apply) const {
		for (unsigned int parent : sets.extensional_graph.vertices[set].parents)
			if (!apply(parent)) return false;
		for (unsigned int parent : sets.intensional_graph.vertices[set].parents)
			if (!apply(parent)) return false;
		return true;
	}

	template<typename Formula>
	inline unsigned int count(const set_reasoning<Formula>& sets, unsigned int set) const {
		return sets.extensional_graph.vertices[set].parents.length
			 + sets.intensional_graph.vertices[set].parents.length;
	}
};

struct singleton_parent {
	unsigned int parent;

	template<typename Formula, typename Function>
	bool for_each_parent(const set_reasoning<Formula>& sets, unsigned int set, Function apply) {
		return apply(parent);
	}

	template<typename Formula>
	constexpr unsigned int count(const set_reasoning<Formula>& sets, unsigned int set) const {
		return 1;
	}
};

template<int MinPriority, typename Formula, typename SetParents>
bool find_largest_disjoint_clique_with_set(
		const set_reasoning<Formula>& sets,
		unsigned int set, const SetParents& set_parents,
		unsigned int*& clique, unsigned int& clique_count,
		unsigned int& ancestor_of_clique)
{
	/* first collect all ancestors of `set` */
	unsigned int parent_count = set_parents.count(sets, set);
	array<unsigned int> stack((parent_count == 0) ? 1 : (1 << (log2(parent_count) + 1)));
	hash_map<unsigned int, array<unsigned int>> non_ancestor_neighborhood(32);
	auto init_non_ancestor_neighborhood = [&](unsigned int current) {
		if (!non_ancestor_neighborhood.check_size())
			return false;

		bool contains; unsigned int bucket;
		array<unsigned int>& siblings = non_ancestor_neighborhood.get(current, contains, bucket);
		if (!contains) {
			if (!array_init(siblings, 4)) return false;
			non_ancestor_neighborhood.table.keys[bucket] = current;
			non_ancestor_neighborhood.table.size++;
		}
		return true;
	};
	if (!init_non_ancestor_neighborhood(set)) {
		for (auto entry : non_ancestor_neighborhood) free(entry.value);
		return false;
	}
	set_parents.for_each_parent(sets, set, [&](unsigned int parent) { stack[stack.length++] = parent; return true; });
	while (stack.length > 0) {
		unsigned int current = stack.pop();
		if (!init_non_ancestor_neighborhood(current)) {
			for (auto entry : non_ancestor_neighborhood) free(entry.value);
			return false;
		}

		for (unsigned int parent : sets.extensional_graph.vertices[current].parents) {
			if (non_ancestor_neighborhood.table.contains(parent)) continue;
			if (!stack.add(parent)) {
				for (auto entry : non_ancestor_neighborhood) free(entry.value);
				return false;
			}
		} for (unsigned int parent : sets.intensional_graph.vertices[current].parents) {
			if (non_ancestor_neighborhood.table.contains(parent)) continue;
			if (!stack.add(parent)) {
				for (auto entry : non_ancestor_neighborhood) free(entry.value);
				return false;
			}
		}
	}

	auto process_ancestor_parent = [&](unsigned int parent) {
		bool parent_changed = false;
		if (!add_non_ancestor_neighbors(sets, parent, parent_changed, non_ancestor_neighborhood))
			return false;
		if (parent_changed && !stack.add(parent)) return false;
		return true;
	};
	if (!set_parents.for_each_parent(sets, set, process_ancestor_parent)) {
		for (auto entry : non_ancestor_neighborhood) free(entry.value);
		return false;
	}
	while (stack.length > 0) {
		unsigned int current = stack.pop();
		for (unsigned int parent : sets.extensional_graph.vertices[current].parents) {
			if (!process_ancestor_parent(parent)) {
				for (auto entry : non_ancestor_neighborhood) free(entry.value);
				return false;
			}
		} for (unsigned int parent : sets.intensional_graph.vertices[current].parents) {
			if (!process_ancestor_parent(parent)) {
				for (auto entry : non_ancestor_neighborhood) free(entry.value);
				return false;
			}
		}
	}

	bool contains; unsigned int bucket;
	free(non_ancestor_neighborhood.get(set, contains, bucket));
	non_ancestor_neighborhood.remove_at(bucket);

	search_queue<ancestor_clique_search_state> queue;
	hash_map<unsigned int, hash_set<unsigned int>> descendants(64);
	clique = NULL; clique_count = 0;
	int best_clique_score = INT_MIN;
	for (const auto& entry : non_ancestor_neighborhood) {
		ancestor_clique_search_state initial_state;
		initial_state.ancestor = entry.key;
		unsigned int* completed_clique = NULL; unsigned int completed_clique_count;
		if (!init(initial_state.clique_state, sets, NULL, 0, entry.value.data, entry.value.length, NULL, 0, set, INT_MAX)) {
			for (auto entry : descendants) free(entry.value);
			for (auto entry : non_ancestor_neighborhood) free(entry.value);
			return false;
		} else if (!process_clique_search_state<false, true, false, MinPriority>(
				queue, sets, initial_state.clique_state, descendants,
				completed_clique, completed_clique_count, initial_state.ancestor))
		{
			free(initial_state);
			for (auto entry : descendants) free(entry.value);
			for (auto entry : non_ancestor_neighborhood) free(entry.value);
			return false;
		}
		free(initial_state);

		if (completed_clique != NULL) {
			/* this only happens if `set` is disjoint with everything in the initial neighborhood */
			if (clique != NULL) free(clique);
			clique = completed_clique;
			clique_count = completed_clique_count;
			ancestor_of_clique = entry.key;
			best_clique_score = sets.sets[set].set_size - sets.sets[entry.key].set_size;
		}
	}
	for (auto entry : non_ancestor_neighborhood) free(entry.value);

	bool success = true;
	for (unsigned int iteration = 0; success && !queue.is_empty() && queue.priority() > best_clique_score; iteration++) {
		search_state<ancestor_clique_search_state> next = queue.pop(iteration);
		unsigned int* completed_clique = NULL; unsigned int completed_clique_count;
		success = process_clique_search_state<true, true, true, MinPriority>(
				queue, sets, next.state->clique_state, descendants,
				completed_clique, completed_clique_count, next.state->ancestor);

		if (completed_clique != NULL) {
			if (queue.last_priority > best_clique_score) {
				if (clique != NULL) free(clique);
				clique = completed_clique;
				clique_count = completed_clique_count;
				ancestor_of_clique = next.state->ancestor;
				best_clique_score = queue.last_priority;
			} else {
				free(completed_clique);
			}
		}
		free(next);
	}

	for (auto entry : descendants) free(entry.value);
	return success;
}

template<int MinPriority, typename Formula>
inline bool find_largest_disjoint_clique_with_set(
		const set_reasoning<Formula>& sets, unsigned int set,
		unsigned int*& clique, unsigned int& clique_count,
		unsigned int& ancestor_of_clique)
{
	default_parents parents;
	return find_largest_disjoint_clique_with_set<MinPriority>(sets, set, parents, clique, clique_count, ancestor_of_clique);
}

template<int MinPriority, typename Formula>
inline bool find_largest_disjoint_clique_with_set(
		const set_reasoning<Formula>& sets,
		unsigned int set, unsigned int parent,
		unsigned int*& clique, unsigned int& clique_count,
		unsigned int& ancestor_of_clique)
{
	singleton_parent set_parent = { parent };
	return find_largest_disjoint_clique_with_set<MinPriority>(sets, set, set_parent, clique, clique_count, ancestor_of_clique);
}

#endif /* SET_GRAPH_H_ */