<?xml version='1.0' encoding='UTF-8' standalone='yes' ?>
<tagfile doxygen_version="1.12.0">
  <compound kind="file">
    <name>Dummy.hpp</name>
    <path>uzuki2/</path>
    <filename>Dummy_8hpp.html</filename>
    <includes id="interfaces_8hpp" name="interfaces.hpp" local="yes" import="no" module="no" objc="no">interfaces.hpp</includes>
    <class kind="class">uzuki2::DummyExternals</class>
    <namespace>uzuki2</namespace>
  </compound>
  <compound kind="file">
    <name>interfaces.hpp</name>
    <path>uzuki2/</path>
    <filename>interfaces_8hpp.html</filename>
    <class kind="class">uzuki2::Base</class>
    <class kind="class">uzuki2::Vector</class>
    <class kind="class">uzuki2::IntegerVector</class>
    <class kind="class">uzuki2::NumberVector</class>
    <class kind="class">uzuki2::StringVector</class>
    <class kind="class">uzuki2::BooleanVector</class>
    <class kind="class">uzuki2::Factor</class>
    <class kind="class">uzuki2::Nothing</class>
    <class kind="class">uzuki2::External</class>
    <class kind="class">uzuki2::List</class>
    <namespace>uzuki2</namespace>
  </compound>
  <compound kind="file">
    <name>parse_hdf5.hpp</name>
    <path>uzuki2/</path>
    <filename>parse__hdf5_8hpp.html</filename>
    <includes id="interfaces_8hpp" name="interfaces.hpp" local="yes" import="no" module="no" objc="no">interfaces.hpp</includes>
    <includes id="Dummy_8hpp" name="Dummy.hpp" local="yes" import="no" module="no" objc="no">Dummy.hpp</includes>
    <includes id="ParsedList_8hpp" name="ParsedList.hpp" local="yes" import="no" module="no" objc="no">ParsedList.hpp</includes>
    <class kind="struct">uzuki2::hdf5::Options</class>
    <namespace>uzuki2</namespace>
    <namespace>uzuki2::hdf5</namespace>
  </compound>
  <compound kind="file">
    <name>parse_json.hpp</name>
    <path>uzuki2/</path>
    <filename>parse__json_8hpp.html</filename>
    <includes id="interfaces_8hpp" name="interfaces.hpp" local="yes" import="no" module="no" objc="no">interfaces.hpp</includes>
    <includes id="Dummy_8hpp" name="Dummy.hpp" local="yes" import="no" module="no" objc="no">Dummy.hpp</includes>
    <includes id="ParsedList_8hpp" name="ParsedList.hpp" local="yes" import="no" module="no" objc="no">ParsedList.hpp</includes>
    <class kind="struct">uzuki2::json::Options</class>
    <namespace>uzuki2</namespace>
    <namespace>uzuki2::json</namespace>
  </compound>
  <compound kind="file">
    <name>ParsedList.hpp</name>
    <path>uzuki2/</path>
    <filename>ParsedList_8hpp.html</filename>
    <includes id="interfaces_8hpp" name="interfaces.hpp" local="yes" import="no" module="no" objc="no">interfaces.hpp</includes>
    <class kind="struct">uzuki2::ParsedList</class>
    <namespace>uzuki2</namespace>
  </compound>
  <compound kind="file">
    <name>uzuki2.hpp</name>
    <path>uzuki2/</path>
    <filename>uzuki2_8hpp.html</filename>
    <includes id="parse__json_8hpp" name="parse_json.hpp" local="yes" import="no" module="no" objc="no">parse_json.hpp</includes>
    <namespace>uzuki2</namespace>
  </compound>
  <compound kind="class">
    <name>uzuki2::Base</name>
    <filename>classuzuki2_1_1Base.html</filename>
    <member kind="function" virtualness="pure">
      <type>virtual Type</type>
      <name>type</name>
      <anchorfile>classuzuki2_1_1Base.html</anchorfile>
      <anchor>a37447b97260aa3a4dd591a71aaad70b8</anchor>
      <arglist>() const =0</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>uzuki2::BooleanVector</name>
    <filename>classuzuki2_1_1BooleanVector.html</filename>
    <base>uzuki2::Vector</base>
    <member kind="function">
      <type>Type</type>
      <name>type</name>
      <anchorfile>classuzuki2_1_1BooleanVector.html</anchorfile>
      <anchor>a6408d4f39ad93f71ca2b8b7b0e3770ff</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual void</type>
      <name>set</name>
      <anchorfile>classuzuki2_1_1BooleanVector.html</anchorfile>
      <anchor>a57c21c43d4c7404f7f61eb98a1cc9d50</anchor>
      <arglist>(size_t i, bool v)=0</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>uzuki2::DummyExternals</name>
    <filename>classuzuki2_1_1DummyExternals.html</filename>
  </compound>
  <compound kind="class">
    <name>uzuki2::External</name>
    <filename>classuzuki2_1_1External.html</filename>
    <base>uzuki2::Base</base>
    <member kind="function">
      <type>Type</type>
      <name>type</name>
      <anchorfile>classuzuki2_1_1External.html</anchorfile>
      <anchor>af6b8774268202cee6e8718714b13a095</anchor>
      <arglist>() const</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>uzuki2::Factor</name>
    <filename>classuzuki2_1_1Factor.html</filename>
    <base>uzuki2::Vector</base>
    <member kind="function">
      <type>Type</type>
      <name>type</name>
      <anchorfile>classuzuki2_1_1Factor.html</anchorfile>
      <anchor>a5e9e95109053bfcbb91a30aad6fb5476</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual void</type>
      <name>set</name>
      <anchorfile>classuzuki2_1_1Factor.html</anchorfile>
      <anchor>a8ef2ea4bf336ab2527afb1b89d01711f</anchor>
      <arglist>(size_t i, size_t v)=0</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual void</type>
      <name>set_level</name>
      <anchorfile>classuzuki2_1_1Factor.html</anchorfile>
      <anchor>a4e76f892adf9041b6f01cedb0e12f740</anchor>
      <arglist>(size_t il, std::string vl)=0</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>uzuki2::IntegerVector</name>
    <filename>classuzuki2_1_1IntegerVector.html</filename>
    <base>uzuki2::Vector</base>
    <member kind="function">
      <type>Type</type>
      <name>type</name>
      <anchorfile>classuzuki2_1_1IntegerVector.html</anchorfile>
      <anchor>a54a45807198d1947c9ed2a49ec772d60</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual void</type>
      <name>set</name>
      <anchorfile>classuzuki2_1_1IntegerVector.html</anchorfile>
      <anchor>ad6806f779fa8d4a41cc97b14765c80b3</anchor>
      <arglist>(size_t i, int32_t v)=0</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>uzuki2::List</name>
    <filename>classuzuki2_1_1List.html</filename>
    <base>uzuki2::Base</base>
    <member kind="function">
      <type>Type</type>
      <name>type</name>
      <anchorfile>classuzuki2_1_1List.html</anchorfile>
      <anchor>a89494054001ac9e355d65596eb692181</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual size_t</type>
      <name>size</name>
      <anchorfile>classuzuki2_1_1List.html</anchorfile>
      <anchor>a8ffb41fd40c9578911b82d7463642cba</anchor>
      <arglist>() const =0</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual void</type>
      <name>set</name>
      <anchorfile>classuzuki2_1_1List.html</anchorfile>
      <anchor>a308225edf101962afc751caa321f5f82</anchor>
      <arglist>(size_t i, std::shared_ptr&lt; Base &gt; v)=0</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual void</type>
      <name>set_name</name>
      <anchorfile>classuzuki2_1_1List.html</anchorfile>
      <anchor>a16d57dcda0cb9c8ffa7724d16473a4ff</anchor>
      <arglist>(size_t i, std::string n)=0</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>uzuki2::Nothing</name>
    <filename>classuzuki2_1_1Nothing.html</filename>
    <base>uzuki2::Base</base>
    <member kind="function">
      <type>Type</type>
      <name>type</name>
      <anchorfile>classuzuki2_1_1Nothing.html</anchorfile>
      <anchor>a61876879ffc5fe14f31252d6b9aee439</anchor>
      <arglist>() const</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>uzuki2::NumberVector</name>
    <filename>classuzuki2_1_1NumberVector.html</filename>
    <base>uzuki2::Vector</base>
    <member kind="function">
      <type>Type</type>
      <name>type</name>
      <anchorfile>classuzuki2_1_1NumberVector.html</anchorfile>
      <anchor>a41ff944db6e679a3dc685130d215d322</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual void</type>
      <name>set</name>
      <anchorfile>classuzuki2_1_1NumberVector.html</anchorfile>
      <anchor>aeaa2342479571f738e039600accef8b5</anchor>
      <arglist>(size_t i, double v)=0</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>uzuki2::hdf5::Options</name>
    <filename>structuzuki2_1_1hdf5_1_1Options.html</filename>
    <member kind="variable">
      <type>hsize_t</type>
      <name>buffer_size</name>
      <anchorfile>structuzuki2_1_1hdf5_1_1Options.html</anchorfile>
      <anchor>a7d44084f355c6def4b8109f69b9a15cd</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>strict_list</name>
      <anchorfile>structuzuki2_1_1hdf5_1_1Options.html</anchorfile>
      <anchor>a93ab8f17a23f6677921101b86626541e</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>uzuki2::json::Options</name>
    <filename>structuzuki2_1_1json_1_1Options.html</filename>
    <member kind="variable">
      <type>bool</type>
      <name>parallel</name>
      <anchorfile>structuzuki2_1_1json_1_1Options.html</anchorfile>
      <anchor>a9e42303766d0747c2b47e6f5f60d3443</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>strict_list</name>
      <anchorfile>structuzuki2_1_1json_1_1Options.html</anchorfile>
      <anchor>a81e2aaee989fdceb801cd0069f0d2b1a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>buffer_size</name>
      <anchorfile>structuzuki2_1_1json_1_1Options.html</anchorfile>
      <anchor>ab3a08f0ea9132c8ae2980049b928b84d</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>uzuki2::ParsedList</name>
    <filename>structuzuki2_1_1ParsedList.html</filename>
    <member kind="variable">
      <type>Version</type>
      <name>version</name>
      <anchorfile>structuzuki2_1_1ParsedList.html</anchorfile>
      <anchor>a1b5e6358d899e983b810d2e7088b235f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::shared_ptr&lt; Base &gt;</type>
      <name>ptr</name>
      <anchorfile>structuzuki2_1_1ParsedList.html</anchorfile>
      <anchor>a59daad773fe543ae299ff51ce78b2f5e</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>uzuki2::StringVector</name>
    <filename>classuzuki2_1_1StringVector.html</filename>
    <base>uzuki2::Vector</base>
    <member kind="enumeration">
      <type></type>
      <name>Format</name>
      <anchorfile>classuzuki2_1_1StringVector.html</anchorfile>
      <anchor>a19642038725cf2a60f158245c876cab6</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>Type</type>
      <name>type</name>
      <anchorfile>classuzuki2_1_1StringVector.html</anchorfile>
      <anchor>a2f366149232cd6df0dea2e2f0000c7c9</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual void</type>
      <name>set</name>
      <anchorfile>classuzuki2_1_1StringVector.html</anchorfile>
      <anchor>aa2f0f5ebb2988b6c819286b44ba344a5</anchor>
      <arglist>(size_t i, std::string v)=0</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>uzuki2::Vector</name>
    <filename>classuzuki2_1_1Vector.html</filename>
    <base>uzuki2::Base</base>
    <member kind="function" virtualness="pure">
      <type>virtual size_t</type>
      <name>size</name>
      <anchorfile>classuzuki2_1_1Vector.html</anchorfile>
      <anchor>abd47d52bccdf710e1f4958585736d6a2</anchor>
      <arglist>() const =0</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual void</type>
      <name>set_name</name>
      <anchorfile>classuzuki2_1_1Vector.html</anchorfile>
      <anchor>a42f996bcbb75853274f004a29fe2da8f</anchor>
      <arglist>(size_t i, std::string n)=0</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual void</type>
      <name>set_missing</name>
      <anchorfile>classuzuki2_1_1Vector.html</anchorfile>
      <anchor>aa03f82a088edd3c3280be33c6a8c9c41</anchor>
      <arglist>(size_t i)=0</arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>uzuki2</name>
    <filename>namespaceuzuki2.html</filename>
    <namespace>uzuki2::hdf5</namespace>
    <namespace>uzuki2::json</namespace>
    <class kind="class">uzuki2::Base</class>
    <class kind="class">uzuki2::BooleanVector</class>
    <class kind="class">uzuki2::DummyExternals</class>
    <class kind="class">uzuki2::External</class>
    <class kind="class">uzuki2::Factor</class>
    <class kind="class">uzuki2::IntegerVector</class>
    <class kind="class">uzuki2::List</class>
    <class kind="class">uzuki2::Nothing</class>
    <class kind="class">uzuki2::NumberVector</class>
    <class kind="struct">uzuki2::ParsedList</class>
    <class kind="class">uzuki2::StringVector</class>
    <class kind="class">uzuki2::Vector</class>
    <member kind="enumeration">
      <type></type>
      <name>Type</name>
      <anchorfile>namespaceuzuki2.html</anchorfile>
      <anchor>a20eb2837279aae2ade751f07c62b3736</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_vector</name>
      <anchorfile>namespaceuzuki2.html</anchorfile>
      <anchor>a66472c68f697e7f6e1ea576a56cb2de8</anchor>
      <arglist>(Type t)</arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>uzuki2::hdf5</name>
    <filename>namespaceuzuki2_1_1hdf5.html</filename>
    <class kind="struct">uzuki2::hdf5::Options</class>
    <member kind="function">
      <type>ParsedList</type>
      <name>parse</name>
      <anchorfile>namespaceuzuki2_1_1hdf5.html</anchorfile>
      <anchor>a44c67c6de55c5b336e6fb49579cf0f55</anchor>
      <arglist>(const H5::Group &amp;handle, Externals_ ext, const Options &amp;options)</arglist>
      <docanchor file="namespaceuzuki2_1_1hdf5.html" title="Provisioner requirements">provisioner-contract</docanchor>
      <docanchor file="namespaceuzuki2_1_1hdf5.html" title="Externals requirements">external-contract</docanchor>
    </member>
    <member kind="function">
      <type>ParsedList</type>
      <name>parse</name>
      <anchorfile>namespaceuzuki2_1_1hdf5.html</anchorfile>
      <anchor>a7a6cbc1d27b48ce3a862001a1948ed33</anchor>
      <arglist>(const std::string &amp;file, const std::string &amp;name, Externals_ ext, Options options=Options())</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>validate</name>
      <anchorfile>namespaceuzuki2_1_1hdf5.html</anchorfile>
      <anchor>a2c3b06098aebbda4fcf51d32d8b340f4</anchor>
      <arglist>(const H5::Group &amp;handle, int num_external, const Options &amp;options)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>validate</name>
      <anchorfile>namespaceuzuki2_1_1hdf5.html</anchorfile>
      <anchor>aff1c31322ccc10abce8b9fb1b7da9b61</anchor>
      <arglist>(const std::string &amp;file, const std::string &amp;name, int num_external, const Options &amp;options)</arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>uzuki2::json</name>
    <filename>namespaceuzuki2_1_1json.html</filename>
    <class kind="struct">uzuki2::json::Options</class>
    <member kind="function">
      <type>ParsedList</type>
      <name>parse</name>
      <anchorfile>namespaceuzuki2_1_1json.html</anchorfile>
      <anchor>a416088ee97d2e5cc206beae48499a5fb</anchor>
      <arglist>(byteme::Reader &amp;reader, Externals_ ext, const Options &amp;options)</arglist>
    </member>
    <member kind="function">
      <type>ParsedList</type>
      <name>parse_file</name>
      <anchorfile>namespaceuzuki2_1_1json.html</anchorfile>
      <anchor>ae55e948302f34d1c87bfa273ff82a287</anchor>
      <arglist>(const std::string &amp;file, Externals_ ext, const Options &amp;options)</arglist>
    </member>
    <member kind="function">
      <type>ParsedList</type>
      <name>parse_buffer</name>
      <anchorfile>namespaceuzuki2_1_1json.html</anchorfile>
      <anchor>aa8e32ee9c2facc04314af9042c446173</anchor>
      <arglist>(const unsigned char *buffer, size_t len, Externals_ ext, const Options &amp;options)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>validate</name>
      <anchorfile>namespaceuzuki2_1_1json.html</anchorfile>
      <anchor>a5c8b849f53a52df5e2158823f60a3dd3</anchor>
      <arglist>(byteme::Reader &amp;reader, int num_external, const Options &amp;options)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>validate_file</name>
      <anchorfile>namespaceuzuki2_1_1json.html</anchorfile>
      <anchor>a31a730921744bfda7f2b390b51ec624e</anchor>
      <arglist>(const std::string &amp;file, int num_external, const Options &amp;options)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>validate_buffer</name>
      <anchorfile>namespaceuzuki2_1_1json.html</anchorfile>
      <anchor>a539849702748af88933ba7707e45a71c</anchor>
      <arglist>(const unsigned char *buffer, size_t len, int num_external, const Options &amp;options)</arglist>
    </member>
  </compound>
  <compound kind="page">
    <name>index</name>
    <title>R lists to HDF5 or JSON</title>
    <filename>index.html</filename>
    <docanchor file="index.html">md__2github_2workspace_2README</docanchor>
  </compound>
</tagfile>
