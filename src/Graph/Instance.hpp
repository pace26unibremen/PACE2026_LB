#ifndef PACE2026_INSTANCE_HPP
#define PACE2026_INSTANCE_HPP

#include "Forest.hpp"

namespace solver
{
    struct Context;
}

namespace graph
{
    /// \typedef Instance
    /// \brief An instance of the MAF Problem is a vector of forests.
    using Instance = std::vector<std::shared_ptr<Forest>>;

    /// \brief Reads an instance from a file.
    /// Expects newick format.
    /// \param path to newick file
    std::shared_ptr<Instance> ReadInstance(const std::filesystem::path& path);

    /// \brief Reads an instance from a file, additionally parsing the lower-bound-track
    /// "#a {a} {b}" line into \p context (see \ref ReadInstance(std::istream&, const std::shared_ptr<solver::Context>&)).
    /// \param path to newick file
    /// \param context context whose a/b fields are populated if a "#a" line is present
    std::shared_ptr<Instance> ReadInstance(const std::filesystem::path& path,
                                           const std::shared_ptr<solver::Context>& context);

    /// \brief Reads an instance from a stream.
    /// Expects newick format.
    /// \param inputStream of instances
    std::shared_ptr<Instance> ReadInstance(std::istream& inputStream);

    /// \brief Reads an instance from a stream, additionally parsing the lower-bound-track
    /// "#a {a} {b}" line (a = validity multiplier, b = offset) into \p context if present and
    /// non-null. The multiplier is stored both as a double (Context::a) and as the exact
    /// rational Context::aNumerator / Context::aScale taken from its decimal digits.
    /// \param inputStream the stream to read from
    /// \param context context to populate, or nullptr to ignore the "#a" line
    std::shared_ptr<Instance> ReadInstance(std::istream& inputStream,
                                           const std::shared_ptr<solver::Context>& context);

    /// \brief Writes an instance to a stream.
    /// Writes in newick format.
    /// \param instance the instance to write
    /// \param os the outstream
    void WriteInstance(const std::shared_ptr<Instance>& instance, std::ostream& os);

    /// \brief Writes an instance to a file.
    /// Writes in newick format.
    /// \param instance the instance to write
    /// \param path to file
    void WriteInstance(const std::shared_ptr<Instance>& instance, const std::filesystem::path& path);

    /// \brief Writes an instance as dot graph to a stream.
    /// \param instance the instance to write
    /// \param os the outstream
    void DotInstance(const std::shared_ptr<Instance>& instance, std::ostream& os);

    /// \brief Writes an instance as dot graph to a file.
    /// \param instance the instance to write
    /// \param path to file
    void DotInstance(const std::shared_ptr<Instance>& instance, const std::filesystem::path& path);
}

#endif  //PACE2026_INSTANCE_HPP
