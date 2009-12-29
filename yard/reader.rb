require 'yard'

# Class to process <tt>embedded_reader :foo</tt> constructs
class YARD::Handlers::Ruby::ParameterHandler < YARD::Handlers::Ruby::Base

  handles method_call(:embedded_reader)

  def process

    statement.parameters.each do |p|
      break unless p
      name = case p.type
             when :symbol_literal
               p.jump(:ident, :op, :kw, :const).source
             when :string_literal
               p.jump(:string_content).source
             else
               raise YARD::Parser::UndocumentableError, p.type.to_s
             end

      a = namespace.attributes[scope][name] = SymbolHash[:read => nil]
      a[:read] = MethodObject.new(namespace, name, scope) do |o|
        o.visibility = :public
        o.signature = "def #{name}"
        o.source = "def #{name}\n  @backend.#{name}\nend"
        o.explicit = false
      end
      register a[:read]

    end
  end

end
