#pragma once

namespace clap {
class Polymorphic {
  public:
	virtual ~Polymorphic() = default;

	Polymorphic() = default;
	Polymorphic(Polymorphic const&) = default;
	Polymorphic(Polymorphic&&) = default;
	Polymorphic& operator=(Polymorphic const&) = default;
	Polymorphic& operator=(Polymorphic&&) = default;
};
} // namespace clap
