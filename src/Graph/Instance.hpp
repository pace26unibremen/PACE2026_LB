#ifndef PACE2026_INSTANCE_HPP
#define PACE2026_INSTANCE_HPP

#include "Forest.hpp"

namespace graph
{
    /// \typedef Instance
    /// \brief An instance of the MAF Problem is a vector of forests.
    using Instance = std::vector<std::shared_ptr<Forest>>;

    /// \brief Reads an instance from a file.
    /// Expects newick format.
    /// \param path to newick file
    std::shared_ptr<Instance> ReadInstance(const std::filesystem::path& path);

    /// \brief Reads an instance from a stdin.
    /// Expects newick format.
    /// \param inputStream of instances
    std::shared_ptr<Instance> ReadInstance(std::istream& inputStream);

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
