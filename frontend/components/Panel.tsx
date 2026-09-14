export default function Panel({
  title,
  action,
  children,
  className = "",
}: {
  title?: string;
  action?: React.ReactNode;
  children: React.ReactNode;
  className?: string;
}) {
  return (
    <section className={`panel p-4 sm:p-5 ${className}`}>
      {(title || action) && (
        <div className="mb-4 flex items-center justify-between border-b border-line pb-3">
          {title && <h2 className="label-tech">{title}</h2>}
          {action}
        </div>
      )}
      {children}
    </section>
  );
}
