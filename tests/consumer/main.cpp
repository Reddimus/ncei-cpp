#include <ncei/ncei.hpp>
int main() {
	ncei::Dataset out;
	const ncei::Result<void> result = ncei::deserialize_dataset(R"({"id":"GHCND"})", out);
	return result && out.id == "GHCND" ? 0 : 1;
}
