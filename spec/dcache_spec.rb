require "dcache"

share_examples_for "base DCache" do
  it "should retrieve 'value'" do
    c.add :key, "value"
    c.get(:key).should == "value"
  end

  it "bunch retrieve simple" do
    (1..500).each do |i|
      c.add :"key#{i}", "value#{i}"
      c.get(:"key#{i}").should == "value#{i}"
    end
  end

  it "bunch retrieve delayed" do
    (1..500).each do |i|
      c.add :"key#{i}", "value#{i}"
    end
    (100..300).each do |i|
      c.get(:"key#{i}").should == "value#{i}"
    end
  end

  it "should retrieve nil" do
    c.get(:foo).should be_nil
  end

  it "should retrieve 123" do
    c.get(:foo, 123).should == 123
  end

  it "should execute" do
    executed = false
    c.get(:foo) do
      executed = true
      :ok
    end.should == :ok
    executed.should be_true
  end

  it "should add value" do
    c.get_or_add(:key, "value").should == "value"
    c.get(:key).should == "value"
  end

  it "should add value from block" do
    c.get_or_add(:key) { "value" }.should == "value"
    c.get(:key).should == "value"
  end

  it "should raise an exception" do
    lambda { c.get_or_raise(:foo) }.should raise_error(ArgumentError)
  end

  it "should remove a key" do
    [:a, :b, :c].each { |i| c.add i, "value" }
    c.delete(:b).should be_nil
    c.get(:b).should be_nil
    [:a, :c].each { |i| c.get(i).should == "value" }
  end

  it "should erase cache" do
    [:a, :b, :c].each { |i| c.add i, "value" }
    c.clear
    [:a, :b, :c].each { |i| c.get(i).should be_nil }
  end

  it "should create a hash" do
    hash = { :a => "value", :b => 42, :c => nil }
    hash.each { |k,v| c.add k, v }
    c.to_hash.should == hash
  end

  it "should time out immediately" do
    c.add :key, "value", -1
    c.get(:key).should be_nil
  end

  it "should time out" do
    c.add :key, "value", 1
    sleep 1
    c.get(:key).should be_nil
  end

  it "hits should be 0" do
    c.get :foo
    c.hits.should == 0
  end

  it "hits should be 1" do
    c.add :key, 3
    c.get :key
    c.hits.should == 1
  end

  it "misses should be 0" do
    c.add :key, 3
    c.get :key
    c.misses.should == 0
  end

  it "misses should be 1" do
    c.get :foo
    c.misses.should == 1
  end

  it "'s lenght should be 0" do
    c.length.should == 0
  end

  it "'s length should be 1" do
    c.add :key, 1
    c.length.should == 1
  end

  it "'s length should be 1000" do
    (1..1100).each { |i| c.add :"key#{i}", i }
    c.length.should == 1000
    c.to_hash.size.should == 1000
  end

end

##############################################################################
# Specific tests below

describe "DCache(:ary)" do
  let(:c) { DCache.new :ary, 1000 }
  it_should_behave_like "base DCache"
end

describe "DCache(:list)" do
  let(:c) { DCache.new :list, 1000 }
  it_should_behave_like "base DCache"

  it "stored should be 0" do
    c.stored.should == 0
  end

  it "stored should be 1" do
    c.add :key, 3
    c.stored.should == 1
  end

  it "removes should be 0" do
    c.removed.should == 0
  end

  it "removed should be 1" do
    c.add :key, 3
    c.clear
    c.removed.should == 1
  end
  it "removed should be 1" do
    c.add :key, 3
    c.remove :key
    c.removed.should == 1
  end

  it "should remove excess items" do
    (1..10).each { |i| c.add i, :a }
    a = [ 2, 8, 9, 1, 5, 10, 4, 3, 7, 6 ]
    a.each { |i| c.get i }
    (c.max_items = 5).should == 5
    c.max_items.should == 5
    c.length.should == 5
    a[0..4].each { |i| c.get(i).should be_nil }
    a[5..9].each { |i| c.get(i).should == :a }
  end
end
