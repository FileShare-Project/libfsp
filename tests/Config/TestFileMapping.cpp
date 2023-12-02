/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Wed Nov 22 20:19:02 2023 Francois Michaut
** Last update Mon Nov 27 21:26:38 2023 Francois Michaut
**
** TestFileMapping.cpp : FileMapping classes implementation
*/

#include "FileShare/FileMapping.hpp"

#include <cassert>

using namespace FileShare;

static void test_virtual_type();
static void test_host_type();
static void test_set_type();
static void test_set_name();

static void test_host_to_virtual();
static void test_find_virtual_node();

int Config_TestFileMapping(int, char **)
{
    // PathNode testing
    test_virtual_type();
    test_host_type();
    test_set_type();
    test_set_name();

    // FileMapping testing
    test_host_to_virtual();
    test_find_virtual_node();
    return 0;
}

static void test_virtual_type()
{
    std::vector<PathNode> nodes = {
        PathNode::make_virtual_node("test"),
        PathNode::make_virtual_node("test", PathNode::HIDDEN),
        PathNode::make_virtual_node("test", PathNode::VISIBLE),
        PathNode::make_virtual_node("test", PathNode::HIDDEN, {PathNode::make_virtual_node("test2")}),
        PathNode::make_virtual_node("test", PathNode::VISIBLE, {PathNode::make_virtual_node("test2")}),
    };

    for (auto &node : nodes) {
        assert(node.get_type() == PathNode::VIRTUAL);
        assert(node.get_host_path().empty());

        try {
            node.set_host_path("test");
            assert(false); // Never reaches this point
        } catch (std::runtime_error &e) {
        }
    }
}

static void test_host_type()
{
    std::vector<PathNode> nodes = {
        PathNode::make_host_node("test", PathNode::HOST_FILE, "/some/path", PathNode::HIDDEN),
        PathNode::make_host_node("test", PathNode::HOST_FILE, "/some/path", PathNode::VISIBLE),
        PathNode::make_host_node("test", PathNode::HOST_FOLDER, "/some/other/path", PathNode::HIDDEN),
        PathNode::make_host_node("test", PathNode::HOST_FOLDER, "/some/other/path", PathNode::VISIBLE),
    };

    for (auto &node : nodes) {
        assert(node.get_type() != PathNode::VIRTUAL);
        assert(node.get_child_nodes().empty());

        try {
            node.set_child_nodes({node});
            assert(false); // Never reaches this point
        } catch (std::runtime_error &e) {
        }

        try {
            node.add_child_node(node);
            assert(false); // Never reaches this point
        } catch (std::runtime_error &e) {
        }
    }
}

static void test_set_type() {
    PathNode host_node = PathNode::make_host_node("test", PathNode::HOST_FILE, "/some_file");
    PathNode virtual_node = PathNode::make_virtual_node("test", {host_node});

    host_node.set_type(PathNode::HOST_FOLDER);
    host_node.set_type(PathNode::HOST_FILE);
    try {
        host_node.set_type(PathNode::VIRTUAL);
        assert(false); // Never reaches this point
    } catch (std::runtime_error &e) {
    }
    host_node.set_host_path("");
    host_node.set_type(PathNode::VIRTUAL);

    try {
        virtual_node.set_type(PathNode::HOST_FILE);
        assert(false); // Never reaches this point
    } catch (std::runtime_error &e) {
    }
    try {
        virtual_node.set_type(PathNode::HOST_FOLDER);
        assert(false); // Never reaches this point
    } catch (std::runtime_error &e) {
    }
    virtual_node.clear_child_nodes();
    virtual_node.set_type(PathNode::HOST_FILE);
    virtual_node.set_type(PathNode::HOST_FOLDER);
}

static void test_set_name() {
    PathNode node = PathNode::make_virtual_node("test");

    assert(node.get_name() == "test");
    // trims trailing /
    node.set_name("////test2////");
    assert(node.get_name() == "test2");

    try {
        // Crashes if / in middle of name
        node.set_name("test/test2");
        assert(false);
    } catch (std::runtime_error &e) {}
}

static void test_host_to_virtual() {
    std::vector<PathNode> root_nodes = {
        PathNode::make_virtual_node("home", PathNode::VISIBLE, {
            PathNode::make_virtual_node("username", PathNode::VISIBLE, {
                // Visible file inside a visible folder -> should succeed
                {PathNode::make_host_node("test1", PathNode::HOST_FILE, "/home/user1/test", PathNode::VISIBLE)},
                // Hidden file inside a visible folder -> should fail
                {PathNode::make_host_node("test2", PathNode::HOST_FILE, "/home/user2/test", PathNode::HIDDEN)},
                // Make sure we don't find this in place of "user/test2"
                {PathNode::make_host_node("us", PathNode::HOST_FOLDER, "/home/us", PathNode::VISIBLE)},
                // Visible folder inside a visible folder -> should succeed for the folder and anything inside it
                {PathNode::make_host_node("downloads", PathNode::HOST_FOLDER, "/home/user/downloads", PathNode::VISIBLE)},
                // Hidden folder inside a visible folder -> should fail for the folder and anything inside it
                {PathNode::make_host_node("documents", PathNode::HOST_FOLDER, "/home/user/documents", PathNode::HIDDEN)}
            })
        }),
        PathNode::make_virtual_node("etc", PathNode::VISIBLE, {
            // Visible file inside a visible folder -> should succeed
            {PathNode::make_host_node("fstab", PathNode::HOST_FILE, "/etc/fstab", PathNode::VISIBLE)},
            // Hidden file inside a visible folder -> should fail
            {PathNode::make_host_node("passwd", PathNode::HOST_FILE, "/etc/passwd", PathNode::HIDDEN)}
        }),
        PathNode::make_virtual_node("root", PathNode::HIDDEN, {
            // Visible file inside a hidden folder -> should fail
            {PathNode::make_host_node("test1", PathNode::HOST_FILE, "/root/test1", PathNode::VISIBLE)},
            // Hidden file inside a hidden folder -> should fail
            {PathNode::make_host_node("test2", PathNode::HOST_FILE, "/root/toto/test2", PathNode::HIDDEN)}
        }),
        // Visible folder -> should succeed for the folder and anything inside it
        PathNode::make_host_node("tmp", PathNode::HOST_FOLDER, "/tmp", PathNode::VISIBLE),
        // Hidden folder -> should fail for the folder and anything inside it
        PathNode::make_host_node("opt", PathNode::HOST_FOLDER, "/opt", PathNode::HIDDEN),
    };

    FileMapping mapping("//fsp", root_nodes);

    assert(mapping.host_to_virtual("/non-existent/path").empty());
    assert(mapping.host_to_virtual("/home").empty());
    assert(mapping.host_to_virtual("/home/user1").empty());
    assert(mapping.host_to_virtual("/home/user1/test") == "/fsp/home/username/test1");
    assert(mapping.host_to_virtual("/home/user1/test/").empty());
    assert(mapping.host_to_virtual("/home/user2").empty());
    assert(mapping.host_to_virtual("/home/user2/test").empty());
    assert(mapping.host_to_virtual("/home/us") == "/fsp/home/username/us");
    assert(mapping.host_to_virtual("/home/us/something/else") == "/fsp/home/username/us/something/else");
    assert(mapping.host_to_virtual("/home/user/downloads") == "/fsp/home/username/downloads");
    assert(mapping.host_to_virtual("/home/user/downloads/some/random/path") == "/fsp/home/username/downloads/some/random/path");
    assert(mapping.host_to_virtual("/home/user/documents").empty());
    assert(mapping.host_to_virtual("/home/user/documents/some/random/path").empty());

    assert(mapping.host_to_virtual("/etc").empty());
    assert(mapping.host_to_virtual("/etc/fake").empty());
    assert(mapping.host_to_virtual("/etc/fstab") == "/fsp/etc/fstab");
    assert(mapping.host_to_virtual("/etc/passwd").empty());

    assert(mapping.host_to_virtual("/root").empty());
    assert(mapping.host_to_virtual("/root/test1").empty());
    assert(mapping.host_to_virtual("/root/test2").empty());
    assert(mapping.host_to_virtual("/root/toto/test2").empty());

    assert(mapping.host_to_virtual("/tmp") == "/fsp/tmp");
    assert(mapping.host_to_virtual("/tmp/") == "/fsp/tmp/");
    assert(mapping.host_to_virtual("/tmp/yolo/") == "/fsp/tmp/yolo/");
    assert(mapping.host_to_virtual("/tmp/yolo/foo/bar") == "/fsp/tmp/yolo/foo/bar");

    assert(mapping.host_to_virtual("/opt").empty());
    assert(mapping.host_to_virtual("/opt/yolo/").empty());
    assert(mapping.host_to_virtual("/opt/yolo/foo/bar").empty());
}

static void test_find_virtual_node() {
    PathNode test1_node = PathNode::make_host_node("test1", PathNode::HOST_FILE, "/home/user1/test", PathNode::VISIBLE);
    PathNode test2_node = PathNode::make_host_node("test2", PathNode::HOST_FILE, "/home/user2/test", PathNode::HIDDEN);
    PathNode downloads_node = PathNode::make_host_node("downloads", PathNode::HOST_FOLDER, "/home/user/downloads", PathNode::VISIBLE);
    PathNode documents_node = PathNode::make_host_node("documents", PathNode::HOST_FOLDER, "/home/user/documents", PathNode::HIDDEN);
    PathNode username_node = PathNode::make_virtual_node("username", PathNode::VISIBLE, {
        // Visible file inside a visible folder -> should succeed
        {test1_node},
        // Hidden file inside a visible folder -> should fail
        {test2_node},
        // Visible folder inside a visible folder -> should succeed for the folder and anything inside it
        {downloads_node},
        // Hidden folder inside a visible folder -> should fail for the folder and anything inside it
        {documents_node}
    });
    PathNode home_node = PathNode::make_virtual_node("home", PathNode::VISIBLE, {username_node});

    PathNode passwd_node = PathNode::make_host_node("passwd", PathNode::HOST_FILE, "/etc/passwd", PathNode::HIDDEN);
    PathNode fstab_node = PathNode::make_host_node("fstab", PathNode::HOST_FILE, "/etc/fstab", PathNode::VISIBLE);
    PathNode etc_node = PathNode::make_virtual_node("etc", PathNode::VISIBLE, {
        // Visible file inside a visible folder -> should succeed
        {fstab_node},
        // Hidden file inside a visible folder -> should fail
        {passwd_node}
    });
    PathNode root_node = PathNode::make_virtual_node("root", PathNode::HIDDEN, {
        // Visible file inside a hidden folder -> should fail
        {PathNode::make_host_node("test1", PathNode::HOST_FILE, "/root/test1", PathNode::VISIBLE)},
        // Hidden file inside a hidden folder -> should fail
        {PathNode::make_host_node("test2", PathNode::HOST_FILE, "/root/toto/test2", PathNode::HIDDEN)}
    });
    PathNode tmp_node = PathNode::make_host_node("tmp", PathNode::HOST_FOLDER, "/tmp", PathNode::VISIBLE);
    PathNode opt_node = PathNode::make_host_node("opt", PathNode::HOST_FOLDER, "/opt", PathNode::HIDDEN);
    std::vector<PathNode> root_nodes = {
        home_node,
        etc_node,
        root_node,
        // Visible folder -> should succeed for the folder and anything inside it
        tmp_node,
        // Hidden folder -> should fail for the folder and anything inside it
        opt_node,
    };

    FileMapping mapping(root_nodes);

    assert(mapping.find_virtual_node("//fsp/non-existent/path").has_value() == false);
    assert(mapping.find_virtual_node("//fsp/home/") == home_node);
    assert(mapping.find_virtual_node("//fsp/home/test1").has_value() == false);
    assert(mapping.find_virtual_node("//fsp/home/username") == username_node);
    assert(mapping.find_virtual_node("//fsp/home/username/test1") == test1_node);
    assert(mapping.find_virtual_node("//fsp/home/username/test2").has_value() == false);
    assert(mapping.find_virtual_node("//fsp/home/username/downloads") == downloads_node);
    assert(mapping.find_virtual_node("//fsp/home/username/downloads/some/random/path") == downloads_node);
    assert(mapping.find_virtual_node("//fsp/home/username/documents").has_value() == false);
    assert(mapping.find_virtual_node("//fsp/home/user/documents/some/random/path").has_value() == false);

    assert(mapping.find_virtual_node("//fsp/etc") == etc_node);
    assert(mapping.find_virtual_node("//fsp/etc/") == etc_node);
    assert(mapping.find_virtual_node("//fsp/etc/fake").has_value() == false);
    assert(mapping.find_virtual_node("//fsp/etc/fstab") == fstab_node);
    assert(mapping.find_virtual_node("//fsp/etc/passwd").has_value() == false);

    assert(mapping.find_virtual_node("//fsp/root").has_value() == false);
    assert(mapping.find_virtual_node("//fsp/root/test1").has_value() == false);
    assert(mapping.find_virtual_node("//fsp/root/test2").has_value() == false);
    assert(mapping.find_virtual_node("//fsp/root/toto/test2").has_value() == false);

    assert(mapping.find_virtual_node("//fsp/tmp") == tmp_node);
    assert(mapping.find_virtual_node("//fsp/tmp/") == tmp_node);
    assert(mapping.find_virtual_node("//fsp/tmp/yolo/") == tmp_node);
    assert(mapping.find_virtual_node("//fsp/tmp/yolo/foo/bar") == tmp_node);

    assert(mapping.find_virtual_node("//fsp/opt").has_value() == false);
    assert(mapping.find_virtual_node("//fsp/opt/yolo/").has_value() == false);
    assert(mapping.find_virtual_node("//fsp/opt/yolo/foo/bar").has_value() == false);

    assert(mapping.find_virtual_node("") == mapping.get_root_node());
    assert(mapping.find_virtual_node("//fsp") == mapping.get_root_node());
    assert(mapping.find_virtual_node("//fsp/") == mapping.get_root_node());
}
