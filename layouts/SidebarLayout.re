/*
    This is the master layout for displaying sidebar based API docs.
    Most of the modules defined in here are here to be reused
    in other API related layouts, such as the Markdown representation
    or the Sidebar component.
 */

%raw
"require('../styles/main.css')";

%raw
{|
let hljs = require('highlight.js/lib/highlight');
let reasonHighlightJs = require('reason-highlightjs');
hljs.registerLanguage('reason', reasonHighlightJs);
|};

open Util.ReactStuff;
module Link = Next.Link;


module UrlPath = {
  /*
      Example base: /apis/javascript
      Example route: /apis/javascript/latest/belt/something/mutable-map-int

      would parse into following `t`:
      {
       base: "/apis/javascript",
       version: "latest",
       relPaths: [|"something"|],
       up: Some("belt"),
       current: "mutable-map-int"
      }
   */
  type t = {
    base: string,
    version: string,
    relPaths: array(string),
    up: option(string),
    current: option(string),
  };

  let parse = (~base: string, route: string): option(t) => {
    let allPaths =
      Js.String2.replace(route, base ++ "/", "")->Js.String2.split("/");

    let total = Belt.Array.length(allPaths);
    if (total < 2) {
      None;
    } else {
      let version = Belt.Array.getExn(allPaths, 0);
      let (up, current) =
        switch (Js.Array2.slice(allPaths, ~end_=total, ~start=-2)) {
        | [|up, current|] =>
          let up = up === version ? None : Some(up);
          (up, Some(current));
        | _ => (None, None)
        };

      let relPaths = Js.Array.slice(allPaths, ~start=1, ~end_=-2);

      Some({base, relPaths, version, up, current});
    };
  };

  /* Beautifies current titles from the url representation */
  let prettyString = (str: string) => {
    Util.String.(str->camelCase->capitalize);
  };

  let fullUpLink = (urlPath: t): string => {
    let {base, up, version} = urlPath;
    base
    ++ "/"
    ++ version
    ++ up->Belt.Option.mapWithDefault("", str => "/" ++ str);
  };

  type breadcrumb = {
    name: string,
    href: string,
  };

  /*
      Example to represent:
      Api / JavaScript / latest / Belt / Array

      ~prefix=[{name: "API", href="apis"}, {name: "JavaScript", href="apis/javascript/latest"}]

   */
  let toBreadCrumbs =
      (~prefix: list(breadcrumb)=[], urlPath: t): list(breadcrumb) => {
    let {base, version, relPaths, up} = urlPath;

    let upCrumb =
      Belt.Option.mapWithDefault(up, [], up =>
        [{name: prettyString(up), href: fullUpLink(urlPath)}]
      );

    let calculatedCrumbs =
      Belt.List.(
        concat(
          fromArray(relPaths),
          urlPath.current
          ->Belt.Option.mapWithDefault([], current => [current]),
        )
        ->map(path => {
            let upPath = Belt.Option.mapWithDefault(up, "", up => up ++ "/");
            {
              name: prettyString(path),
              href: base ++ "/" ++ version ++ "/" ++ upPath ++ path,
            };
          })
      );
    Belt.List.(concatMany([|prefix, upCrumb, calculatedCrumbs|]));
  };
};

module BreadCrumbs = {
  // See UrlPath for more details on the parameters
  [@react.component]
  let make = (~crumbs: list(UrlPath.breadcrumb)) => {
    <div className="w-full overflow-x-auto text-xs text-night">
      {Belt.List.mapWithIndex(
         crumbs,
         (i, crumb) => {
           let item =
             if (i === Belt.List.length(crumbs) - 1) {
               <span key={Belt.Int.toString(i)}> crumb.name->s </span>;
             } else {
               <Link key={Belt.Int.toString(i)} href={crumb.href}>
                 <a> crumb.name->s </a>
               </Link>;
             };
           if (i > 0) {
             <span key={Belt.Int.toString(i)}> " / "->s item </span>;
           } else {
             item;
           };
         },
       )
       ->Belt.List.toArray
       ->ate}
    </div>;
  };
};

module Sidebar = {
  module Title = {
    [@react.component]
    let make = (~children) => {
      let className = "font-sans font-black text-night-dark text-xl mt-5";

      <div className> children </div>;
    };
  };

  module NavItem = {
    // Navigation point information
    type t = {
      name: string,
      href: string,
    };
    [@react.component]
    let make =
        (
          ~isItemActive: t => bool=_nav => false,
          ~isHidden=false,
          ~items: array(t),
        ) => {
      <ul className="ml-2 mt-1 text-night">
        {Belt.Array.map(
           items,
           m => {
             let hidden = isHidden ? "hidden" : "block";
             let active =
               isItemActive(m)
                 ? {j| bg-primary-15 text-primary-dark rounded -mx-2 px-2 font-bold block |j}
                 : "";
             <li
               key={m.name}
               className={hidden ++ " leading-5 w-4/5"}
               // to make non-interactive elements (like div, span or li) tab-able
               // see https://developer.mozilla.org/en-US/docs/Web/Accessibility/Keyboard-navigable_JavaScript_widgets
               tabIndex=0>
               <Link href={m.href}>
                 <a
                   className={
                     "truncate block h-8 md:h-auto text-night hover:text-primary "
                     ++ active
                   }>
                   m.name->s
                 </a>
               </Link>
             </li>;
           },
         )
         ->ate}
      </ul>;
    };
  };

  module Category = {
    type t = {
      name: string,
      items: array(NavItem.t),
    };

    [@react.component]
    let make = (~isItemActive: option(NavItem.t => bool)=?, ~category: t) => {
      <div key={category.name} className="my-12">
        <Title> category.name->s </Title>
        <NavItem ?isItemActive items={category.items} />
      </div>;
    };
  };

  module ToplevelNav = {
    [@react.component]
    let make = (~title="", ~backHref=?, ~version=?) => {
      let back =
        switch (backHref) {
        | Some(href) =>
          <Link href>
            <a className="w-5 h-5">
              <Icon.CornerLeftUp className="w-full h-full" />
            </a>
          </Link>
        | None => React.null
        };

      let versionTag =
        switch (version) {
        | Some(version) => <Tag kind=`Subtle text=version />
        | None => React.null
        };

      <div className="flex items-center justify-between mb-4 w-full">
        <div className="flex items-center w-2/3">
          back
          <span className="ml-2 font-sans font-black text-night-dark text-xl">
            title->s
          </span>
        </div>
        <div className="ml-auto"> versionTag </div>
      </div>;
    };
  };

  module CollapsibleSection = {
    module NavUl = {
      // Navigation point information
      type t = {
        name: string,
        href: string,
      };

      [@react.component]
      let make =
          (
            ~onItemClick: option(ReactEvent.Mouse.t => unit)=?,
            ~isItemActive: t => bool=_nav => false,
            ~items: array(t),
          ) => {
        <ul className="mt-3 text-night">
          {Belt.Array.map(
             items,
             m => {
               let active =
                 isItemActive(m)
                   ? {j| bg-primary-15 text-primary-dark -ml-1 px-2 font-bold block |j}
                   : "";
               <li
                 key={m.name}
                 className="leading-5 w-4/5"
                 // to make non-interactive elements (like div, span or li) tab-able
                 // see https://developer.mozilla.org/en-US/docs/Web/Accessibility/Keyboard-navigable_JavaScript_widgets
                 tabIndex=0>
                 <a
                   href={m.href}
                   onClick=?onItemClick
                   className={
                     "truncate block pl-3 h-8 md:h-auto border-l-2 border-night-10 block text-night hover:pl-4 hover:text-night-dark"
                     ++ active
                   }>
                   m.name->s
                 </a>
               </li>;
             },
           )
           ->ate}
        </ul>;
      };
    };
    [@react.component]
    let make =
        (
          ~onHeaderClick: option(ReactEvent.Mouse.t => unit)=?,
          ~isItemActive=?,
          ~headers: array(string),
          ~moduleName: string,
        ) => {
      let (collapsed, setCollapsed) = React.useState(() => false);
      let items =
        Belt.Array.map(headers, header =>
          NavUl.{name: header, href: "#" ++ header}
        );

      let direction = collapsed ? `Down : `Up;

      <div className="py-3 px-3 bg-primary-15 rounded-lg">
        <a
          className="flex justify-between items-center cursor-pointer text-primary hover:text-primary text-night-dark text-base"
          href="#"
          onClick={evt => {
            ReactEvent.Mouse.preventDefault(evt);
            setCollapsed(isCollapsed => !isCollapsed);
          }}>
          moduleName->s
          <span className="ml-2 block text-primary">
            <Icon.Caret size=`Md direction />
          </span>
        </a>
        {if (!collapsed) {
           <NavUl ?isItemActive onItemClick=?onHeaderClick items />;
         } else {
           React.null;
         }}
      </div>;
    };
  };

  // subitems: list of functions inside given module (defined by route)
  [@react.component]
  let make =
      (
        ~categories: array(Category.t),
        ~route: string,
        ~toplevelNav=React.null,
        ~preludeSection=React.null,
        ~isOpen: bool,
        ~toggle: unit => unit,
      ) => {
    let isItemActive = (navItem: NavItem.t) => {
      navItem.href === route;
    };

    <>
      <div
        className={
          (isOpen ? "fixed w-full left-0 h-full z-10 min-w-20" : "hidden ")
          ++ " md:block md:w-1/4 md:h-auto md:relative overflow-y-visible bg-white md:relative"
        }>
        <aside
          className="relative top-0 px-4 w-full block md:top-16 md:sticky border-r border-snow-dark overflow-y-auto scrolling-touch pb-24 pt-8"
          style={Style.make(~height="calc(100vh - 4rem", ())}>
          <div className="flex justify-between">
            <div className="w-3/4 md:w-full"> toplevelNav </div>
            <button
              onClick={evt => {
                ReactEvent.Mouse.preventDefault(evt);
                toggle();
              }}
              className="md:hidden h-8">
              <Icon.Close />
            </button>
          </div>
          preludeSection
          /* Firefox ignores padding in scroll containers, so we need margin
               to make a bottom gap for the sidebar.
               See https://stackoverflow.com/questions/29986977/firefox-ignores-padding-when-using-overflowscroll
             */
          <div className="mb-56">
            {categories
             ->Belt.Array.map(category =>
                 <div key={category.name}>
                   <Category isItemActive category />
                 </div>
               )
             ->ate}
          </div>
        </aside>
      </div>
    </>;
  };
};

module MobileDrawerButton = {
  [@react.component]
  let make = (~hidden: bool, ~onClick) => {
    <button
      className={(hidden ? "hidden" : "") ++ " md:hidden mr-3"}
      onMouseDown=onClick>
      <img className="h-4" src="/static/ic_sidebar_drawer.svg" />
    </button>;
  };
};

/*
    sidebarOpen: shows if sidebar is open for mobile view
    toggleSidebar: toggles sidebar for mobile view
 */
[@react.component]
let make =
    (
      ~theme: ColorTheme.t,
      ~components: Mdx.Components.t,
      // (Sidebar, toggleSidebar)
      ~sidebar: (React.element, unit => unit),
      ~breadcrumbs: option(list(UrlPath.breadcrumb))=?,
      ~route: string,
      ~children,
    ) => {
  let (isOpen, setIsOpen) = React.useState(() => false);

  let theme = ColorTheme.toCN(theme);

  let breadcrumbs =
    breadcrumbs->Belt.Option.mapWithDefault(React.null, crumbs =>
      <BreadCrumbs crumbs />
    );

  let (sidebar, toggleSidebar) = sidebar;
  <>
    <Meta />
    <div className={"mt-16 min-w-20 " ++ theme}>
      <div className="w-full text-night font-base">
        <Navigation
          isOverlayOpen=isOpen
          toggle={() => setIsOpen(prev => !prev)}
          route
        />
        <div className="flex justify-center">
          <div className="lg:align-center w-full max-w-xl">
            <Mdx.Provider components>
              <div className="flex">
                sidebar
                <div
                  className="flex justify-center w-full md:w-3/4 overflow-hidden">
                  <main className="w-5/6 pt-10 mb-32 text-lg">
                    <div
                      className="fixed border-b shadow top-16 left-0 pl-4 bg-white w-full py-4 md:relative md:border-none md:shadow-none md:p-0 md:top-auto flex items-center">
                      <MobileDrawerButton
                        hidden=isOpen
                        onClick={evt => {
                          ReactEvent.Mouse.preventDefault(evt);
                          toggleSidebar();
                        }}
                      />
                      <div className="truncate overflow-x-auto touch-scroll">
                        breadcrumbs
                      </div>
                    </div>
                    <div className="mt-10"> children </div>
                  </main>
                </div>
              </div>
            </Mdx.Provider>
          </div>
        </div>
      </div>
    </div>
  </>;
};